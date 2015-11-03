#pragma once

#include "config.h"
#include "noncopyable.h"
#include "thread_start.h"

namespace util
{

class thread : private noncopyable
{
public:
	thread();
	~thread();

	bool start(void (*proc)(void*), void* param)
	{
		return start(thread_start(proc, param));
	}

	template<typename Type, typename R>
	bool start(Type* object, R (Type::*method)())
	{
		return start(thread_start(object, method));
	}

	bool join();

private:
	bool start(const thread_start& tstart);

#if defined(_MSC_VER)
	unsigned tid_;
	HANDLE handle_;
#elif defined(__GNUC__)
	pthread_t handle_;
	bool created_;
#endif
};

}
