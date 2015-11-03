#include "mutex.h"

#if defined(__GNUC__)
#  include <pthread.h>
#endif

namespace util
{

#if defined(_MSC_VER)

mutex::mutex()
{
	::InitializeCriticalSection(&csection_);
}

mutex::~mutex()
{
	::DeleteCriticalSection(&csection_);
}

void mutex::lock()
{
	::EnterCriticalSection(&csection_);
}

void mutex::unlock()
{
	::LeaveCriticalSection(&csection_);
}

#elif defined(__GNUC__)

mutex::mutex()
{
	pthread_mutex_destroy(&mutx_);
}

mutex::~mutex()
{
	pthread_mutex_destroy(&mutx_);
}

void mutex::lock()
{
	pthread_mutex_lock(&mutx_);
}

void mutex::unlock()
{
	pthread_mutex_unlock(&mutx_);
}

#endif

}
