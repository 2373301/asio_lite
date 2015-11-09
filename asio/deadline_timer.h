#pragma once

#include "timer_queue.h"

namespace asio
{

class io_service;

class deadline_timer : private util::noncopyable
{
public:
	deadline_timer(io_service& io_service);
	~deadline_timer();

	template<typename Handle>
	void async_wait(long millisec, Handle h)
	{
		io_service_.add_async_timer(*this, millisec, h);
	}
	unsigned int cancel();

private:
	friend class io_service;
	io_service& io_service_;
	per_timer_data per_timer_data_;
};

}
