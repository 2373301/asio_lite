#pragma once

#if defined(_MSC_VER)

#include <thread>
#include <mutex>
#include "timer_queue.h"
#include "socket.h"

namespace asio
{

class deadline_timer;

class iocp_io_service
{
public:
	iocp_io_service();
	~iocp_io_service();

	int register_handle(socket& s);

	template<typename Handle>
	void async_accept(socket& acceptor, socket& new_socket, Handle h)
	{
		typedef io_operation_accept<Handle> op;
		io_operation_accept<Handle>::ptr p;
		p.v = asio_handler_alloc(sizeof(op), &h);
		p.p = new (p.v) op(new_socket, h);

		work_started();
		DWORD bytes_read = 0;
		accept_ex_fn accept_ex = iocp_io_service::get_accept_ex(acceptor);
		BOOL result = accept_ex(acceptor.socket_, new_socket.socket_, p.p->output_buffer_,
			0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes_read, p.p);
		DWORD last_error = ::WSAGetLastError();
		if (!result && last_error != WSA_IO_PENDING)
			on_completion(p.p, last_error, 0);
		else
			on_pending(p.p);
		p.v = p.p = NULL;
	}

	template<typename Handle>
	void async_connect(socket& s, sockaddr* addr, unsigned int addrlen, Handle h)
	{
		typedef io_operation1<Handle> op;
		io_operation1<Handle>::ptr p;
		p.v = asio_handler_alloc(sizeof(op), &h);
		p.p = new (p.v) op(h);

		work_started();
		
		if (addr == 0)
		{
			on_completion(p.p, -1, 0);
			p.v = p.p = NULL;
			return;
		}

		DWORD bytes_read = 0;
		connect_ex_fn connect_ex = iocp_io_service::get_connect_ex(s);
		BOOL result = connect_ex(s.socket_, addr, addrlen, 0, 0, &bytes_read, p.p);
		DWORD last_error = ::WSAGetLastError();
		if (!result && last_error != WSA_IO_PENDING)
			on_completion(p.p, last_error, 0);
		else
			on_pending(p.p);
		p.v = p.p = NULL;
	}

	template<typename Handle>
	void async_read_some(socket& s, void* buff, unsigned int bufflen, Handle h)
	{
		typedef io_operation_wr<Handle> op;
		io_operation_wr<Handle>::ptr p;
		p.v = asio_handler_alloc(sizeof(op), &h);
		p.p = new (p.v) op(buff, bufflen, h);
		
		work_started();

		DWORD bytes_read = 0;
		DWORD flags = 0;
		int result = ::WSARecv(s.socket_, &p.p->buff_, 1, &bytes_read, &flags, p.p, 0);
		DWORD last_error = ::WSAGetLastError();
		if (result != 0 && last_error != WSA_IO_PENDING)
			on_completion(p.p, last_error, 0);
		else
			on_pending(p.p);
		p.v = p.p = NULL;
	}
	
	template<typename Handle>
	void async_write_some(socket& s, void* buff, unsigned int bufflen, Handle h)
	{
		typedef io_operation_wr<Handle> op;
		io_operation_wr<Handle>::ptr p;
		p.v = asio_handler_alloc(sizeof(op), &h);
		p.p = new (p.v) op(buff, bufflen, h);

		work_started();

		DWORD bytes_read = 0;
		int result = ::WSASend(s.socket_, &p.p->buff_, 1, &bytes_read, 0, p.p, 0);
		DWORD last_error = ::WSAGetLastError();
		if (result != 0 && last_error != WSA_IO_PENDING)
			on_completion(p.p, last_error, 0);
		else
			on_pending(p.p);
		p.v = p.p = NULL;
	}

	template<typename Handle>
	void post(Handle h)
	{
		typedef io_operation_post<Handle> op;
		io_operation_post<Handle>::ptr p;
		p.v = asio_handler_alloc(sizeof(op), &h);
		p.p = new (p.v) op(h);
		post_immediate_completion(p.p);
		p.v = p.p = NULL;
	}

	template<typename Handle>
	void add_async_timer(deadline_timer& timer, long millisec, Handle h)
	{
		typedef io_operation1<Handle> op;
		io_operation1<Handle>::ptr p;
		p.v = asio_handler_alloc(sizeof(op), &h);
		p.p = new (p.v) op(h);

		if (::InterlockedExchangeAdd(&shutdown_, 0) != 0)
		{
			post_immediate_completion(p.p);
			return;
		}

		std::unique_lock<std::mutex> lock(dispatch_mutex_);
		if (timer_event_ == NULL)
		{
			timer_event_ = ::CreateWaitableTimer(NULL, FALSE, NULL );
            std::thread t(&iocp_io_service::timer_thread, this);
            timer_thread_.swap(t);
		}
		bool earliest = timer_queue_.enqueue_timer(
			get_time() + millisec, timer.per_timer_data_, p.p);
		p.v = p.p = NULL;

		work_started();
		if (earliest)
		{
			update_timeout();
		}
	}

	unsigned int cancel_timer(deadline_timer& timer);
	void work_started();
	void work_finished();
	void run();

	void stop();
	void reset();

	static long long get_time();
	static accept_ex_fn get_accept_ex(socket& s);
	static accept_ex_socketaddrs_fn get_accept_ex_socketaddrs(socket& s);
	static connect_ex_fn get_connect_ex(socket& s);

private:
	void shutdown_service();
	void update_timeout();

	void on_pending(io_operation* op);
	void on_completion(io_operation* op, DWORD last_error, DWORD bytes_transferred);
	void post_immediate_completion(io_operation* op);
	void post_deferred_completion(io_operation* op);
	void post_deferred_completions(op_queue<io_operation>& ops);
	

private:
	void timer_thread();

	HANDLE iocp_;
	HANDLE timer_event_;
	std::thread timer_thread_;
	long shutdown_;
	long stopped_;
	long stop_event_posted_;
	long outstanding_work_;
	long dispatch_required_;
	std::mutex dispatch_mutex_;
	timer_queue timer_queue_;
	op_queue<io_operation> completed_ops_;
	static accept_ex_fn accept_ex_;
	static accept_ex_socketaddrs_fn accept_ex_socketaddrs_;
	static connect_ex_fn connect_ex_;
};

inline int get_last_error(int default_error = -1)
{
	int last_error = ::WSAGetLastError();
	return last_error != 0 ? last_error : default_error;
}

}

#endif
