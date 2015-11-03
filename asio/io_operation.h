#pragma once

namespace asio
{

#define IO_DEFINE_HANDLER_PTR(op) \
struct ptr \
{ \
	Handler* h; \
	void* v; \
	op* p; \
	~ptr() \
	{ \
      reset(); \
	} \
	void reset()\
	{ \
		if (p) \
		{ \
			p->~op(); \
			p = 0; \
		} \
		if (v) \
		{ \
			asio_handler_dealloc(v, h); \
			v = 0; \
		} \
	} \
}

} // namespace asio

#if defined(_MSC_VER)
#  include "iocp_io_operation.h"
#endif

static void* asio_handler_alloc(unsigned int size, void*)
{
	return ::malloc(size);
}

static void asio_handler_dealloc(void* ptr, void*)
{
	::free(ptr);
}

