#pragma once

#include <util/noncopyable.h>
#include "io_operation.h"

namespace asio
{

class io_service;

class socket : private util::noncopyable
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
		io_service_.async_accept(*this, new_socket, h);
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
			if (host == NULL)
			{
				io_service_.async_connect(*this, 0, 0, h);
				return;
			}
			saddri.sin_addr.s_addr = ((LPIN_ADDR)host->h_addr)->s_addr;		
		}
		memcpy(&remote_addr_, &saddri, sizeof(sockaddr_in));
		io_service_.async_connect(*this, (sockaddr*)&remote_addr_, sizeof(sockaddr_in), h);
	}

	template<typename Handle>
	void async_read_some(void* buff, unsigned int bufflen, Handle h)
	{
		io_service_.async_read_some(*this, buff, bufflen, h);
	}

	template<typename Handle>
	void async_write_some(void* buff, unsigned int bufflen, Handle h)
	{
		io_service_.async_write_some(*this, buff, bufflen, h);
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
	friend class iocp_io_service;
	socket_type socket_;
	io_service& io_service_;
	char remote_addr_[(sizeof(SOCKADDR_IN) + 16)];
};

}
