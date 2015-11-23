#pragma once

#include "noncopyable.h"
#include "io_operation.h"

namespace x
{

class io_service;

class socket 
    : private x::noncopyable
{
public:
	socket(io_service& io_service);
	~socket();

	io_service& get_io_service() {return io_service_;}

	int set_option(int level, int optname, const char* optvalue, int optlen);
	int get_option(int level, int optname, char* optvalue, int* optlen);

	int open(int type = SOCK_STREAM, int protocol = IPPROTO_TCP);
	int close();
	int bind(unsigned short port, const char* ipaddr = NULL);
	int listen(int backlog = SOMAXCONN);

	template<typename Handle>
	void async_accept(socket& new_socket, Handle h)
	{
        typedef io_operation_accept<Handle> op;
        op* p = new op(new_socket, h);

        socket& acceptor = *this;
        io_service_.work_started();
        DWORD bytes_read = 0;
        accept_ex_fn accept_ex = io_service::get_accept_ex(acceptor);
        BOOL result = accept_ex(acceptor.socket_, new_socket.socket_, p->output_buffer_,
            0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes_read, p);
        DWORD last_error = ::WSAGetLastError();
        if (!result && last_error != WSA_IO_PENDING)
            io_service_.on_completion(p, last_error, 0);
        else
            io_service_.on_pending(p);
	}

	template<typename Handle>
	void async_connect(const char* ipaddr, unsigned short port, Handle h)
	{
		sockaddr_in saddri;
		::memset(&saddri, 0, sizeof(saddri));
		saddri.sin_family	= AF_INET;
		saddri.sin_port = ::htons(port);
		saddri.sin_addr.s_addr = ::inet_addr(ipaddr);

		if (saddri.sin_addr.s_addr == INADDR_NONE)
		{
			LPHOSTENT host = ::gethostbyname(ipaddr);
// 			if (host == NULL)
// 			{
// 				async_connect(*this, 0, 0, h);
// 				return;
// 			}
			saddri.sin_addr.s_addr = ((LPIN_ADDR)host->h_addr)->s_addr;		
		}
		memcpy(&remote_addr_, &saddri, sizeof(sockaddr_in));
		//io_service_.async_connect(*this, (sockaddr*)&remote_addr_, sizeof(sockaddr_in), h);

        {   
            socket& s = *this;
            sockaddr* addr = (sockaddr*)&remote_addr_;
            unsigned int addrlen = sizeof(sockaddr_in);

            typedef io_operation1<Handle> op;
            op* p = new op(h);

            io_service_.work_started();

            if (addr == 0)
            {
                io_service_.on_completion(p, -1, 0);
                return;
            }

            DWORD bytes_read = 0;
            connect_ex_fn connect_ex = io_service::get_connect_ex(s);
            BOOL result = connect_ex(s.socket_, addr, addrlen, 0, 0, &bytes_read, p);
            DWORD last_error = ::WSAGetLastError();
            if (!result && last_error != WSA_IO_PENDING)
                io_service_.on_completion(p, last_error, 0);
            else
                io_service_.on_pending(p);
        }
	}

	template<typename Handle>
	void async_read_some(void* buff, unsigned int bufflen, Handle h)
	{
        socket& s = *this;

        typedef io_operation_wr<Handle> op;
        op* p = new op(buff, bufflen, h);

        io_service_.work_started();

        DWORD bytes_read = 0;
        DWORD flags = 0;
        int result = ::WSARecv(s.socket_, &p->buff_, 1, &bytes_read, &flags, p, 0);
        DWORD last_error = ::WSAGetLastError();
        if (result != 0 && last_error != WSA_IO_PENDING)
            io_service_.on_completion(p, last_error, 0);
        else
            io_service_.on_pending(p);
	}

	template<typename Handle>
	void async_write_some(void* buff, unsigned int bufflen, Handle h)
	{
		//io_service_.async_write_some(*this, buff, bufflen, h);
        socket& s = *this;
        typedef io_operation_wr<Handle> op;
        op* p = new op(buff, bufflen, h);

        io_service_.work_started();

        DWORD bytes_read = 0;
        int result = ::WSASend(s.socket_, &p->buff_, 1, &bytes_read, 0, p, 0);
        DWORD last_error = ::WSAGetLastError();
        if (result != 0 && last_error != WSA_IO_PENDING)
            io_service_.on_completion(p, last_error, 0);
        else
            io_service_.on_pending(p);
	}

	template<typename Handle>
	void async_write(const void* buff, unsigned int bufflen, Handle h)
	{
		write_op<Handle> wo(*this, buff, bufflen, h);
		wo(0, 0, 1);
	}

	void set_remote_addr(LPSOCKADDR remote_addr, int remote_addr_length);
	const char* get_remote_ip();
	unsigned short get_remote_port();

private:
	friend class io_service;
	io_service& io_service_;
    socket_type socket_;
	char remote_addr_[(sizeof(SOCKADDR_IN) + 16)]; 
};

}
