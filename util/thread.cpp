#include "thread.h"

#if defined(_MSC_VER)
#  include <process.h>
#elif defined(__GNUC__)
#  include <pthread.h>
#endif

namespace util
{

#if defined(_MSC_VER)

static unsigned _stdcall thread_entry(void* param)
{
	thread_start* start = static_cast<thread_start*>(param);
	(*start)();
	::free(param);
	return 0;
}

thread::thread()
	: handle_(0)
	, tid_(0)
{
}

thread::~thread()
{
	if (handle_ != 0)
	{
		::CloseHandle(handle_);
	}
}

bool thread::start(const thread_start& tstart)
{
	if (handle_ != 0)
	{
		::CloseHandle(handle_);
	}

	thread_start* param =
		static_cast<thread_start*>(::malloc(sizeof(thread_start)));
	if (param == 0)
	{
		return false;
	}

	*param = tstart; // ¿½±´Öµ
	handle_ = (HANDLE)::_beginthreadex(NULL, 0, &thread_entry, param, 0, &tid_);
	if (handle_ == NULL)
	{
		::free(param);
		return false;
	}

	return true;
}

bool thread::join()
{
	if (0 == handle_)
	{
		return false;
	}

	if (tid_ == ::GetCurrentThreadId())
	{
		::CloseHandle(handle_);
		handle_ = NULL;
		return false;
	}

	if (WAIT_OBJECT_0 != ::WaitForSingleObject(handle_, INFINITE))
	{
		return false;
	}

	::CloseHandle(handle_);
	handle_ = NULL;
	tid_ = 0;
	return true;
}

#elif defined(__GNUC__)

thread::thread()
	: created_(false)
{
}

static void* thread_entry(void* param)
{
	thread_start* start = static_cast<thread_start*>(param);
	(*start)();
	::free(param);
	return 0;
}

thread::~thread()
{
	if (created_)
	{
		pthread_detach(handle_);
	}
}

bool thread::start(const thread_start& tstart)
{
	if (created_)
	{
		pthread_detach(handle_);
	}

	thread_start* param =
		static_cast<thread_start*>(::malloc(sizeof(thread_start)));
	if (param == 0)
	{
		return false;
	}

	*param = tstart; // ¿½±´Öµ
	int err = pthread_create(&handle_, NULL, &thread_entry, param);
	if (err != 0)
	{
		::free(param);
		return false;
	}
	created_ = true;

	return true;
}

bool thread::join()
{
	if (!created_)
	{
		return false;
	}

	pthread_t tid = pthread_self();;
	if (tid == handle_)
	{
		created_ = false;
		pthread_detach(handle_);
		return false;
	}

	pthread_join(handle_, NULL);
	pthread_detach(handle_);

	created_ = false;
	return true;
}

#endif

}
