#pragma once

#include <stdlib.h>
#include <memory>

#if defined(_MSC_VER)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

#elif defined(__GNUC__)
#  include <sys/time.h>

#endif
