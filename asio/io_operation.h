#pragma once

#if defined(_MSC_VER)

#include <WinSock2.h>
#include "binder.h"

typedef BOOL (PASCAL *accept_ex_fn)(SOCKET,
	SOCKET, void*, DWORD,DWORD, DWORD, DWORD*, OVERLAPPED*);
typedef BOOL (PASCAL *accept_ex_socketaddrs_fn)(void*,
	DWORD, DWORD, DWORD, sockaddr**, int*, sockaddr**, int*);
typedef BOOL (PASCAL *connect_ex_fn)(SOCKET,
	const sockaddr*, int, void*, DWORD, DWORD*, OVERLAPPED*);

typedef SOCKET socket_type;

namespace asio
{

enum
{
	io_key_null,
	io_key_dispatch,
	io_key_overlapped_contains_result
};

class io_service;
class socket;

class io_operation : public OVERLAPPED
{
public:
	void complete(io_service& owner, int error, unsigned int bytes_transferred)
	{
		func_(&owner, this, error, bytes_transferred);
	}

	void destroy()
	{
		func_(0, this, -1, 0);
	}

	void reset()
	{
		Internal = 0;
		InternalHigh = 0;
		Offset = 0;
		OffsetHigh = 0;
		hEvent = 0;
		ready_ = 0;
		error_ = 0;
	}

protected:
	typedef void (*func_type)(io_service*, io_operation*, int, unsigned int);
	io_operation(func_type f) : func_(f)
	{
		reset();
	}

	friend class op_queue_access;
	friend class timer_queue;
	friend class io_service;
	io_operation* next_;
	func_type func_;
	long ready_;
	int error_;
};

template <typename Handler>
class io_operation_post : public io_operation
{
public:
	//IO_DEFINE_HANDLER_PTR(io_operation_post);

	io_operation_post(Handler h)
		: h_(h), io_operation(&io_operation_post::do_complete){}
	static void do_complete(io_service* owner, io_operation* base, int error, unsigned int)
	{
		io_operation_post* op = static_cast<io_operation_post*>(base);
		binder0_t<Handler> handler(op->h_);

		//ptr p = { &op->h_, op, op };
		//p.reset();
        delete op;

		if (owner)
		{
			handler();
		}
	}

private:
	Handler h_;
};

template <typename Handler>
class io_operation1 : public io_operation
{
public:
	//IO_DEFINE_HANDLER_PTR(io_operation1);

	io_operation1(Handler h) : h_(h), io_operation(&io_operation1::do_complete){}
	static void do_complete(io_service* owner, io_operation* base, int error, unsigned int)
	{
		io_operation1* op = static_cast<io_operation1*>(base);
		binder1_t<Handler, int> handler(op->h_, op->error_ != 0 ? op->error_ : error);

		//ptr p = { &op->h_, op, op };
		//p.reset();
        delete op;

		if (owner)
		{
			handler();
		}
	}

private:
	Handler h_;
};

template <typename Handler>
class io_operation_accept : public io_operation
{
public:
	//IO_DEFINE_HANDLER_PTR(io_operation_accept);

	io_operation_accept(socket& peer, Handler h)
		: peer_(peer), h_(h), io_operation(&io_operation_accept::do_complete){}
	static void do_complete(io_service* owner, io_operation* base, int error, unsigned int)
	{
		io_operation_accept* op = static_cast<io_operation_accept*>(base);
		binder1_t<Handler, int> handler(op->h_, error);

		if (owner && !error)
		{
			LPSOCKADDR local_addr = 0;
			int local_addr_length = 0;
			LPSOCKADDR remote_addr = 0;
			int remote_addr_length = 0;
			accept_ex_socketaddrs_fn accept_ex_socketaddrs = 
				io_service::get_accept_ex_socketaddrs(op->peer_);
			accept_ex_socketaddrs(op->output_buffer_, 0, sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, &local_addr, &local_addr_length,
				&remote_addr, &remote_addr_length);
			//op->peer_.get_io_service().register_handle(op->peer_);
			op->peer_.set_remote_addr(remote_addr, remote_addr_length);
		}

// 		ptr p = { &op->h_, op, op };
// 		p.reset();
        delete op;

		if (owner)
		{
			handler();
		}
	}

private:
    friend class socket;
	socket& peer_;
	char output_buffer_[(sizeof(SOCKADDR_IN) + 16) * 2];
	SOCKADDR_IN addr;
	Handler h_;
};

template <typename Handler>
class io_operation_wr : public io_operation
{
public:
	//IO_DEFINE_HANDLER_PTR(io_operation_wr);
	io_operation_wr(const void* buff, unsigned int bufflen, Handler h)
		: h_(h), io_operation(&io_operation_wr::do_complete)
	{
		buff_.buf = (char*)buff;
		buff_.len = bufflen;
	}
	static void do_complete(io_service* owner, io_operation* base,
		int error, unsigned int bytes_transferred)
	{
		io_operation_wr* op = static_cast<io_operation_wr*>(base);
		binder2_t<Handler, int, unsigned int> 
			handler(op->h_, error, bytes_transferred);

// 		ptr p = { &op->h_, op, op };
// 		p.reset();
        delete op;

		if (owner)
		{
			handler();
		}
	}

private:
    friend class socket;
	Handler h_;
	WSABUF buff_;
};

template <typename Handler>
class write_op
{
public:
	write_op(socket& socket, const void* buff, unsigned int bufflen, Handler h)
		: socket_(socket), total_transferred_(0), h_(h)
	{
		buff_.buf = (char*)buff;
		buff_.len = bufflen;
	}

	void operator()(int error, unsigned int bytes_transferred, bool start = 0)
	{
		if (start == 1)
		{
			socket_.async_write_some(buff_.buf, buff_.len, *this);
		}
		else
		{
			total_transferred_ += bytes_transferred;
			if (error || total_transferred_ >= buff_.len ||
				!error && bytes_transferred == 0)
			{
				h_(error, total_transferred_);
			}
			else
			{
				socket_.async_write_some(buff_.buf + total_transferred_,
					buff_.len - total_transferred_, *this);
			}
		}
	}

private:
	Handler h_;
	socket& socket_;
	unsigned int total_transferred_;
	WSABUF buff_;
};

}

#endif
