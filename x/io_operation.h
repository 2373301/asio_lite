#pragma once

#if defined(_MSC_VER)

#include <WinSock2.h>
#include <stdint.h>
#include "binder.h"

typedef BOOL (PASCAL *LPAcceptEx)(SOCKET,
	SOCKET, void*, DWORD,DWORD, DWORD, DWORD*, OVERLAPPED*);
typedef BOOL (PASCAL *LPGetAcceptExSockaddrs)(void*,
	DWORD, DWORD, DWORD, sockaddr**, int32_t*, sockaddr**, int32_t*);
typedef BOOL (PASCAL *LPConnectEx)(SOCKET,
	const sockaddr*, int32_t, void*, DWORD, DWORD*, OVERLAPPED*);

typedef SOCKET socket_type;

namespace x
{

enum
{
	io_key_null,
	io_key_dispatch,
	io_key_overlapped_contains_result
};

class io_service;
class socket;

class io_operation 
    : public OVERLAPPED
{
public:
	void complete(io_service& owner, int32_t error, uint32_t bytes_transferred)
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
	typedef void (*func_type)(io_service*, io_operation*, int32_t error, uint32_t bytes_transferred);
	io_operation(func_type f) 
        : func_(f)
	{
		reset();
	}

	friend class op_queue_access;
	friend class timer_queue;
	friend class io_service;
	io_operation* next_;
	func_type func_;
	long ready_;
	int32_t error_;
};

// 携带额外0个参数
template <typename Handler>
class io_operation_extra0 
    : public io_operation
{
public:
	io_operation_extra0(Handler h)
		: h_(h)
        , io_operation(&io_operation_extra0::do_complete)
    {}

    /*
        如果将do_complete声明为虚函数，则多数编译器会在对象起始位置放置vptr，
        这样就改变了内存布局，从而不能再把operation对象直接作为OVERLAPPED对象进行传递了
    */
	static void do_complete(io_service* owner, io_operation* base,
        int32_t error, uint32_t bytes_transferred)
	{
		io_operation_extra0* op = static_cast<io_operation_extra0*>(base);
		binder0_t<Handler> handler(op->h_);
        delete op;
		if (owner)
		{
			handler();
		}
	}

private:
	Handler h_;
};

// 额外1个参数
template <typename Handler>
class io_operation_extra1 
    : public io_operation
{
public:
	io_operation_extra1(Handler h) 
        : h_(h)
        , io_operation(&io_operation_extra1::do_complete)
    {}

	static void do_complete(io_service* owner, io_operation* base,
        int32_t error, uint32_t bytes_transferred)
	{
		io_operation_extra1* op = static_cast<io_operation_extra1*>(base);
        int32_t err = op->error_ != 0 ? op->error_ : error;
		binder1_t<Handler, int32_t> handler(op->h_, err);
        delete op;
		if (owner)
		{
			handler();
		}
	}

private:
	Handler h_;
};

// 类似io_operation_extra1, 执行GetAcceptExSockaddrs, 解析remote addr
template <typename Handler>
class io_operation_accept 
    : public io_operation
{
public:
	io_operation_accept(socket& peer, Handler h)
		: peer_(peer)
        , h_(h)
        , io_operation(&io_operation_accept::do_complete)
    {}

	static void do_complete(io_service* owner, io_operation* base,
        int32_t error, uint32_t bytes_transferred)
	{
		io_operation_accept* op = static_cast<io_operation_accept*>(base);
		binder1_t<Handler, int32_t> handler(op->h_, error);

		if (owner && !error)
		{
			LPSOCKADDR local_addr = 0;
			int32_t local_addr_length = 0;
			LPSOCKADDR remote_addr = 0;
			int32_t remote_addr_length = 0;
			LPGetAcceptExSockaddrs accept_ex_socketaddrs = 
				io_service::get_GetAcceptExSockaddrs(op->peer_);
            // 解析 remote addr 
			accept_ex_socketaddrs(op->output_buffer_, 0, sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, &local_addr, &local_addr_length,
				&remote_addr, &remote_addr_length);
			op->peer_.set_remote_addr(remote_addr, remote_addr_length);
		}

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

// 额外2个参数
template <typename Handler>
class io_operation2 
    : public io_operation
{
public:
	io_operation2(const void* buff, uint32_t bufflen, Handler h)
		: h_(h)
        , io_operation(&io_operation2::do_complete)
	{
		buff_.buf = (char*)buff;
		buff_.len = bufflen;
	}

	static void do_complete(io_service* owner, io_operation* base,
		int32_t error, uint32_t bytes_transferred)
	{
		io_operation2* op = static_cast<io_operation2*>(base);
		binder2_t<Handler, int32_t, uint32_t> 
			handler(op->h_, error, bytes_transferred);

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
	write_op(socket& socket, const void* buff, uint32_t bufflen, Handler h)
		:socket_(socket)
        ,total_transferred_(0)
        ,h_(h)
	{
		vec_.buf = (char*)buff;
		vec_.len = bufflen;
	}

	void operator()(int32_t error, uint32_t bytes_transferred, bool start = 0)
	{
		if (start == 1)
		{
			socket_.async_write_some(vec_.buf, vec_.len, *this);
		}
		else
		{
			total_transferred_ += bytes_transferred;
			if (error   // 如果出错
                || total_transferred_ >= vec_.len      // 超出缓存长度
                || !error && bytes_transferred == 0)    // 没写
			{
				h_(error, total_transferred_);
			}
			else
			{
				socket_.async_write_some(vec_.buf + total_transferred_,
					vec_.len - total_transferred_, *this);
			}
		}
	}

private:
	Handler h_;
	socket& socket_;
	uint32_t total_transferred_;
	WSABUF vec_;
};

}

#endif
