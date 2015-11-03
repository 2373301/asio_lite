#pragma once

#include <util/thread.h>
#include <util/mutex.h>
#include "timer_queue.h"

#if defined(_MSC_VER)
#include "iocp_io_service.h"
#endif

namespace asio
{

#if defined(_MSC_VER)
typedef iocp_io_service io_service_impl;
#endif

class deadline_timer;

class io_service : private util::noncopyable
{
public:
	class work;

	template<typename Handle>
	void async_accept(socket& acceptor, socket& new_socket, Handle h)
	{
		impl_.async_accept(acceptor, new_socket, h);
	}

	template<typename Handle>
	void async_connect(socket& s, sockaddr* addr, unsigned int addrlen, Handle h)
	{
		impl_.async_connect(s, addr, addrlen, h);
	}

	template<typename Handle>
	void async_read_some(socket& s, void* buff, unsigned int bufflen, Handle h)
	{
		impl_.async_read_some(s, buff, bufflen, h);
	}
	
	template<typename Handle>
	void async_write_some(socket& s, void* buff, unsigned int bufflen, Handle h)
	{
		impl_.async_write_some(s, buff, bufflen, h);
	}

	template<typename Handle>
	void post(Handle h)
	{
		impl_.post(h);
	}

	template<typename Handle>
	void add_async_timer(deadline_timer& timer, long millisec, Handle h)
	{
		impl_.add_async_timer(timer, millisec, h);
	}

	unsigned int cancel_timer(deadline_timer& timer)
	{
		return impl_.cancel_timer(timer);
	}

	void run()
	{
		impl_.run();
	}

	void stop()
	{
		impl_.stop();
	}

	void reset()
	{
		impl_.reset();
	}

	static long long get_time()
	{
		return io_service_impl::get_time();
	}

private:
	io_service_impl impl_;
	friend class work;
	friend class socket;
};

class io_service::work
{
public:
  explicit work(io_service& io_service) : io_service_(io_service)
  {
	  io_service_.impl_.work_started();
  }

  work(const work& other): io_service_(other.io_service_)
  {
	  io_service_.impl_.work_started();
  }

  ~work()
  {
	  io_service_.impl_.work_finished();
  }

private:
	void operator=(const work& other);

	io_service& io_service_;
};

}
