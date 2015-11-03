#pragma once

#include "atomic_count.h"

namespace util
{

template<class Type>
class shared_ptr
{
public:
	explicit shared_ptr(Type* p = 0)
		: px_(p)
	{
		pn_ = new atomic_count(1);
		if (p != 0)
		{
			do_enable_share(px_, px_, pn_);
		}
	}

	~shared_ptr()
	{
		if(--*pn_ == 0)
		{
			delete px_;
			delete pn_;
		}
	}

	shared_ptr(const shared_ptr& r)
		: px_(r.px_)
	{
		pn_ = r.pn_;
		++*pn_;
	}

	shared_ptr& operator=(const shared_ptr& r)
	{
		shared_ptr(r).swap(*this);
		return *this;
	}

	void reset(Type* p = NULL)
	{
		shared_ptr(p).swap(*this);
	}

	Type& operator*() const
	{
		return *px_;
	}

	Type* operator->() const
	{
		return px_;
	}

	Type* get() const
	{
		return px_;
	}

	long useCount() const
	{
		return *pn_;
	}

	void swap(shared_ptr<Type>& other)
	{
		std::swap(px_, other.px_);
		std::swap(pn_, other.pn_);
	}

private:
	template<class T> friend class enable_share_from_this;
	shared_ptr(Type* px, atomic_count* pn)
		: px_(px), pn_(pn)
	{
		++*pn_;
	}

	Type* px_;
	atomic_count* pn_;
};

template<class T, class U>
	inline bool operator==(const shared_ptr<T>& a, const shared_ptr<U>& b)
{
	return a.get() == b.get();
}

template<class T, class U>
	inline bool operator!=(const shared_ptr<T>& a, const shared_ptr<U>& b)
{
	return a.get() != b.get();
}


template<class Type>
class enable_share_from_this
{
public:
	shared_ptr<Type> share_from_this()
	{
		shared_ptr<Type> p(px_, pn_);
		return p;
	}

private:
	template<class T>
		friend void do_enable_share(enable_share_from_this<T>*, T*, atomic_count*);
	
	Type* px_;
	atomic_count* pn_;
};

template<class T>
	inline void do_enable_share(enable_share_from_this<T>* pe, T* px, atomic_count* pn)
{
	pe->px_ = px;
	pe->pn_ = pn;
}

inline void do_enable_share(...)
{
}

}
