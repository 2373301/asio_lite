#pragma once

#include "noncopyable.h"

namespace util
{

class thread_start
{
public:
	thread_start(void (*proc)(void*), void* param)
	{
		thread_type_ = TYPE_PROC;
		func_proc_ = proc;
		func_param_ = param;
	}

	template<typename Type, typename R>
	thread_start(Type* object, R (Type::*method)())
	{
		thread_type_ = TYPE_METHOD;
		method_obj_ = static_cast<void*>(object);
		method_stor_ = union_cast<method_storage>(method);
		method_proc_ = &method_invoker<Type, R>;
	}

	void operator()()
	{
		if (thread_type_ == TYPE_PROC)
			func_proc_(func_param_);
		else
			method_proc_(this);
	}

private:
	template<typename Type, typename R>
	static void method_invoker(thread_start* threadStart)
	{
		typedef R (Type::*MethodType)(void);
		Type* object = static_cast<Type*>(threadStart->method_obj_);
		MethodType method = union_cast<MethodType>(threadStart->method_stor_);
		(object->*method)();
	}

	template<typename dst_type, typename src_type>
	static dst_type union_cast(src_type src)
	{
		union
		{
			src_type src;
			dst_type dst;
		} u = {src};
		return u.dst;
	}

private:
	enum THREAD_TYPE{ TYPE_PROC, TYPE_METHOD };
	struct method_storage { char storage[sizeof(void*)*4]; };
	typedef void (*function_proc)(void*);
	typedef void (*method_proc)(thread_start*);

	THREAD_TYPE		thread_type_;
	function_proc	func_proc_;
	void*			func_param_;

	method_storage	method_stor_;
	method_proc		method_proc_;
	void*			method_obj_;
};

}
