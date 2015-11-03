#pragma once

#include "config.h"
#include "noncopyable.h"

namespace util
{

class mutex : private noncopyable
{
	friend class condition;
public:
	mutex();
	~mutex();

	void lock();
	void unlock();

private:
#if defined(_MSC_VER)
	CRITICAL_SECTION csection_;
#elif defined(__GNUC__)
	pthread_mutex_t mutx_;
#endif
};

class unique_lock : private noncopyable
{
public:
	unique_lock(mutex& m) : mutex_(m)
	{
		mutex_.lock();
		owns_ = true;
	}
	~unique_lock()
	{
		if (owns_)
		{
			mutex_.unlock();
		}
	}
	void lock()
	{
		owns_ = true;
		mutex_.unlock();
	}
	void unlock()
	{
		owns_ = false;
		mutex_.unlock();
	}
private:
	mutex& mutex_;
	bool owns_;
};

}