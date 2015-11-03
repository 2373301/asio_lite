#pragma once

#include "config.h"

namespace util
{

class timer
{
public:
	timer();
	void restart();
	long long elapsed() const;

private:
#if defined(_MSC_VER)
	LARGE_INTEGER frequency_;
	LARGE_INTEGER start_time_;
#elif defined(__GNUC__)
	timeval start_time_;
#endif
};

}
