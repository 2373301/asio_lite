#include "condition.h"
#include <limits>
#include <exception>

#if defined(__GNUC__)
#  include <pthread.h>
#  include <errno.h>
#endif

namespace util
{

#if defined(_MSC_VER)

static const char* EXCEPTION_MSG = "condition exception";

condition::condition() : gone_(0), blocked_(0), waiting_(0)
{
	gate_ = ::CreateSemaphore(NULL, 1, 1, NULL);
	if (gate_ == NULL)
		throw std::exception(EXCEPTION_MSG);

	queue_ = ::CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	if (queue_ == NULL)
	{
		::CloseHandle(gate_);
		throw std::exception(EXCEPTION_MSG);
	}

	::InitializeCriticalSection(&csection_);
}

condition::~condition()
{
	if (!::CloseHandle(gate_))
	{
		::CloseHandle(queue_);
		throw std::exception(EXCEPTION_MSG);
	}
	if (!::CloseHandle(queue_))
		throw std::exception(EXCEPTION_MSG);

	::DeleteCriticalSection(&csection_);
}

void condition::wait(mutex& mutex)
{
	if (::WaitForSingleObject(gate_, INFINITE) != WAIT_OBJECT_0)
		throw std::exception(EXCEPTION_MSG);

	++blocked_;
	if (!::ReleaseSemaphore(gate_, 1, NULL))
		throw std::exception(EXCEPTION_MSG);

	mutex.unlock();

	if (::WaitForSingleObject(queue_, INFINITE) != WAIT_OBJECT_0)
		throw std::exception(EXCEPTION_MSG);

	::EnterCriticalSection(&csection_);

	unsigned wasWaiting = waiting_;
	unsigned wasGone = gone_;
	if (wasWaiting != 0)
	{
		if (--waiting_ == 0)
		{
			if (blocked_ != 0)
			{
				if (!::ReleaseSemaphore(gate_, 1, NULL))
					throw std::exception(EXCEPTION_MSG);

				wasWaiting = 0;
			}
			else if (gone_ != 0)
			{
				gone_ = 0;
			}
		}
	}
	else if (++gone_ == (UINT_MAX / 2))
	{
		if (::WaitForSingleObject(gate_, INFINITE) != WAIT_OBJECT_0)
			throw std::exception(EXCEPTION_MSG);

		blocked_ -= gone_;
		if (!::ReleaseSemaphore(gate_, 1, NULL))
			throw std::exception(EXCEPTION_MSG);

		gone_ = 0;
	}

	::LeaveCriticalSection(&csection_);

	if (wasWaiting == 1)
	{
		for (; wasGone; --wasGone)
		{
			if (::WaitForSingleObject(queue_, INFINITE) != WAIT_OBJECT_0)
				throw std::exception(EXCEPTION_MSG);
		}
		if (!::ReleaseSemaphore(gate_, 1, NULL))
			throw std::exception(EXCEPTION_MSG);
	}

	mutex.lock();
}

bool condition::timed_wait(mutex& mutex, unsigned milliscd)
{
	if (::WaitForSingleObject(gate_, INFINITE) != WAIT_OBJECT_0)
		throw std::exception(EXCEPTION_MSG);

	++blocked_;
	if (!::ReleaseSemaphore(gate_, 1, NULL))
		throw std::exception(EXCEPTION_MSG);

	mutex.unlock();

	unsigned int res = ::WaitForSingleObject(queue_, milliscd);
	if (res == WAIT_FAILED || res == WAIT_ABANDONED)
		throw std::exception(EXCEPTION_MSG);

	bool ret = (res == WAIT_OBJECT_0);

	::EnterCriticalSection(&csection_);

	unsigned wasWaiting = waiting_;
	unsigned wasGone = gone_;
	if (wasWaiting != 0)
	{
		if (!ret)
		{
			if (blocked_ != 0)
				--blocked_;
			else
				++gone_;
		}
		if (--waiting_ == 0)
		{
			if (blocked_ != 0)
			{
				if (!::ReleaseSemaphore(gate_, 1, NULL))
					throw std::exception(EXCEPTION_MSG);
				wasWaiting = 0;
			}
			else if (gone_ != 0)
			{
				gone_ = 0;
			}
		}
	}
	else if (++gone_ == UINT_MAX / 2)
	{
		if (::WaitForSingleObject(gate_, INFINITE) != WAIT_OBJECT_0)
			throw std::exception(EXCEPTION_MSG);

		blocked_ -= gone_;
		if (!::ReleaseSemaphore(gate_, 1, NULL))
			throw std::exception(EXCEPTION_MSG);

		gone_ = 0;
	}

	::LeaveCriticalSection(&csection_);

	if (wasWaiting == 1)
	{
		for (; wasGone; --wasGone)
		{
			if (::WaitForSingleObject(queue_, INFINITE) != WAIT_OBJECT_0)
				throw std::exception(EXCEPTION_MSG);
		}
		if (!::ReleaseSemaphore(gate_, 1, NULL))
			throw std::exception(EXCEPTION_MSG);
	}
	mutex.lock();
	return ret;
}

void condition::notify_one()
{
	unsigned signals = 0;

	::EnterCriticalSection(&csection_);

	if (waiting_ != 0)
	{
		if (blocked_ == 0)
		{
			::LeaveCriticalSection(&csection_);
			return;
		}

		signals = 1;
		++waiting_;
		--blocked_;
	}
	else
	{
		if (::WaitForSingleObject(gate_, INFINITE) != WAIT_OBJECT_0)
			throw std::exception(EXCEPTION_MSG);

		if (blocked_ > gone_)
		{
			if (gone_ != 0)
			{
				blocked_ -= gone_;
				gone_ = 0;
			}
			signals = waiting_ = 1;
			--blocked_;
		}
		else
		{
			if (!::ReleaseSemaphore(gate_, 1, NULL))
				throw std::exception(EXCEPTION_MSG);
		}
	}

	::LeaveCriticalSection(&csection_);

	if (signals)
	{
		if (!::ReleaseSemaphore(queue_, signals, NULL))
			throw std::exception(EXCEPTION_MSG);
	}
}

void condition::notify_all()
{
	unsigned signals = 0;

	::EnterCriticalSection(&csection_);

	if (waiting_ != 0)
	{
		if (blocked_ == 0)
		{
			::LeaveCriticalSection(&csection_);
			return;
		}

		waiting_ += (signals = blocked_);
		blocked_ = 0;
	}
	else
	{
		if (::WaitForSingleObject(gate_, INFINITE) != WAIT_OBJECT_0)
			throw std::exception(EXCEPTION_MSG);

		if (blocked_ > gone_)
		{
			if (gone_ != 0)
			{
				blocked_ -= gone_;
				gone_ = 0;
			}
			signals = waiting_ = blocked_;
			blocked_ = 0;
		}
		else
		{
			if (!::ReleaseSemaphore(gate_, 1, NULL))
				throw std::exception(EXCEPTION_MSG);
		}
	}

	::LeaveCriticalSection(&csection_);

	if (signals)
	{
		if (!::ReleaseSemaphore(queue_, signals, NULL))
			throw std::exception(EXCEPTION_MSG);
	}
}

#elif defined(__GNUC__)

condition::condition()
{
	pthread_cond_init(&cond_, 0);
}

condition::~condition()
{
	pthread_cond_destroy(&cond_);
}

void condition::wait(mutex& mutex)
{
	pthread_cond_wait(&cond_, &mutex.mutx_);
}

bool condition::timed_wait(mutex& mutex, unsigned milliscd)
{
	timespec abstime;
	timeval now;
	gettimeofday(&now, NULL);
	int nsec = now.tv_usec * 1000 + (milliscd % 1000) * 1000000;
	abstime.tv_nsec = nsec % 1000000000;
	abstime.tv_sec = now.tv_sec + nsec / 1000000000 + milliscd / 1000;

	int ret = pthread_cond_timedwait(&cond_, &mutex.mutx_, &abstime);
	if (ret == ETIMEDOUT)
	{
		return false;
	}
	return true;
}

void condition::notify_one()
{
	pthread_cond_signal(&cond_); 
}

void condition::notify_all()
{
	pthread_cond_broadcast(&cond_); 
}

#endif

}

