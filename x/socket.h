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

    io_service& get_io_service() { return io_service_; }
	int32_t set_option(int32_t level, int32_t optname, const char* optvalue, int32_t optlen);
	int32_t get_option(int32_t level, int32_t optname, char* optvalue, int32_t* optlen);

	int32_t open(int32_t type = SOCK_STREAM, int32_t protocol = IPPROTO_TCP);
	int32_t close();
	int32_t bind(unsigned short port, const char* ipaddr = NULL);
	int32_t listen(int32_t backlog = SOMAXCONN);

	template<typename Handle>
	void async_accept(socket& new_socket, Handle h)
	{
        typedef io_operation_accept<Handle> op;
        op* p = new op(new_socket, h);

        io_service_.work_started();
        DWORD bytes_read = 0;
        LPAcceptEx accept_ex = io_service::get_AcceptEx(*this);
        BOOL result = accept_ex(socket_, new_socket.socket_, p->output_buffer_,
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
			saddri.sin_addr.s_addr = ((LPIN_ADDR)host->h_addr)->s_addr;		
		}
		memcpy(&remote_addr_, &saddri, sizeof(sockaddr_in));
        
        sockaddr* addr = (sockaddr*)&remote_addr_;
        uint32_t addrlen = sizeof(sockaddr_in);

        typedef io_operation_extra1<Handle> op;
        op* p = new op(h);

        io_service_.work_started();
        if (addr == 0)
        {
            io_service_.on_completion(p, -1, 0);
            return;
        }

        DWORD bytes_read = 0;
        LPConnectEx connect_ex = io_service::get_ConnectEx(*this);
        BOOL result = connect_ex(socket_, addr, addrlen, 0, 0, &bytes_read, p);
        DWORD last_error = ::WSAGetLastError();
        if (!result && last_error != WSA_IO_PENDING)
            io_service_.on_completion(p, last_error, 0);
        else
            io_service_.on_pending(p);
	}

	template<typename Handle>
	void async_read_some(void* buff, uint32_t bufflen, Handle h)
	{
        typedef io_operation2<Handle> op;
        op* p = new op(buff, bufflen, h);

        io_service_.work_started();

        DWORD bytes_read = 0;
        DWORD flags = 0;
        int32_t result = ::WSARecv(socket_, &p->buff_, 1, &bytes_read, &flags, p, 0);
        DWORD last_error = ::WSAGetLastError();
        if (result != 0 && last_error != WSA_IO_PENDING)
            io_service_.on_completion(p, last_error, 0);
        else
            io_service_.on_pending(p);
	}

	template<typename Handle>
	void async_write_some(void* buff, uint32_t bufflen, Handle h)
	{
        typedef io_operation2<Handle> op;
        op* p = new op(buff, bufflen, h);

        io_service_.work_started();

        DWORD bytes_read = 0;
        int32_t result = ::WSASend(socket_, &p->buff_, 1, &bytes_read, 0, p, 0);
        DWORD last_error = ::WSAGetLastError();
        if (result != 0 && last_error != WSA_IO_PENDING)
            io_service_.on_completion(p, last_error, 0);
        else
            io_service_.on_pending(p);
	}

	template<typename Handle>
	void async_write(const void* buff, uint32_t bufflen, Handle h)
	{
		write_op<Handle> wo(*this, buff, bufflen, h);
		wo(0, 0, 1);
	}

	void set_remote_addr(LPSOCKADDR remote_addr, int32_t remote_addr_length);
	const char* get_remote_ip();
	unsigned short get_remote_port();

private:
	friend class io_service;
	io_service& io_service_;
    socket_type socket_;
	char remote_addr_[(sizeof(SOCKADDR_IN) + 16)]; 
};

}
