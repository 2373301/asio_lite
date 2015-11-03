#pragma once

#include "config.h"
#include "noncopyable.h"

namespace util
{

class atomic_count : private noncopyable
{
public:
	explicit atomic_count(int v)
		: value_(v)
	{
	}

	int operator++()
	{
#if defined (_MSC_VER)
		return ::InterlockedIncrement((long*)&value_);
#elif defined(__GNUC__)
		return fetch_add(value_, 1) + 1;
#endif
	}

	int operator--()
	{
#if defined (_MSC_VER)
		return ::InterlockedDecrement((long*)&value_);
#elif defined(__GNUC__)
		return fetch_add(value_, -1) - 1;
#endif
	}

	operator int() const
	{
		return static_cast<int const volatile&>(value_);
	}

private:
#if defined(__GNUC__)
	int fetch_add(int volatile& storage, int v)
	{
		__asm__ __volatile__
			(
			"lock; xaddl %0, %1"
			: "+q" (v), "+m" (storage)
			:
		: "cc", "memory"
			);
		return v;
	}
#endif
	int value_;
};

}
