#include "timer.h"

namespace util
{

#if defined(_MSC_VER)

timer::timer()
{
	QueryPerformanceFrequency(&frequency_);
	restart();
}

void timer::restart()
{
	QueryPerformanceCounter(&start_time_);
}

long long timer::elapsed() const
{
	LARGE_INTEGER end_time;
	QueryPerformanceCounter(&end_time);
	return 1000*(end_time.QuadPart - start_time_.QuadPart) / frequency_.QuadPart;
}

#elif defined(__GNUC__)

timer::timer()
{
	restart();
}

void timer::restart()
{
	gettimeofday(&start_time_, NULL);
}

long long timer::elapsed() const
{
	timeval stop_time_;
	gettimeofday(&stop_time_, NULL);
	return (stop_time_.tv_sec - start_time_.tv_sec) * 1000 +
		(stop_time_.tv_usec - start_time_.tv_usec) / 1000;
}

#endif

}
