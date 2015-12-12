#include "socket.h"
#include "io_service.h"

namespace x
{

socket::socket(io_service& io_service)
	: io_service_(io_service)
	, socket_(INVALID_SOCKET)
{
	memset(&remote_addr_, 0, sizeof(remote_addr_));
}

socket::~socket()
{
	close();
}

int32_t socket::set_option(int32_t level, int32_t optname, const char* optvalue, int32_t optlen)
{
	if (::setsockopt(socket_, level, optname, optvalue, optlen) != NO_ERROR)
	{
		return get_last_error(-1);
	}
	return 0;
}

int32_t socket::get_option(int32_t level, int32_t optname, char* optvalue, int32_t* optlen)
{
	if (::getsockopt(socket_, level, optname, optvalue, optlen) != NO_ERROR)
	{
		return get_last_error(-1);
	}
	return 0;
}

int32_t socket::open(int32_t type, int32_t protocol)
{
	::SetLastError(0);
	socket_ = ::WSASocket(AF_INET, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socket_ == INVALID_SOCKET)
	{
		return get_last_error(-1);
	}
	io_service_.register_handle(*this);
	return 0;
}

int32_t socket::close()
{
	if (socket_ != INVALID_SOCKET)
	{
		::SetLastError(0);
		::closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
	return 0;
}

int32_t socket::bind(unsigned short port, const char* ipaddr)
{
	sockaddr_in saddri;
	::memset(&saddri, 0, sizeof(saddri));
	::SetLastError(0);
	saddri.sin_family = AF_INET;
	saddri.sin_port = ::htons(port);
	if (ipaddr == NULL)
	{
		saddri.sin_addr.s_addr = ::htonl(INADDR_ANY);
	}
	else
	{
		saddri.sin_addr.s_addr = ::inet_addr(ipaddr);
		if (saddri.sin_addr.s_addr == INADDR_NONE)
		{
			LPHOSTENT host = ::gethostbyname(ipaddr);
			if (host == NULL)
			{
				return get_last_error(-1);
			}
			saddri.sin_addr.s_addr = ((LPIN_ADDR)host->h_addr)->s_addr;		
		}
	}
	if (::bind(socket_, (SOCKADDR*)&saddri, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		return get_last_error(-1);
	}
	return 0;
}

int32_t socket::listen(int32_t backlog)
{
	::SetLastError(0);
	if (::listen(socket_, backlog) == SOCKET_ERROR)
	{
		return get_last_error(-1);
	}
	return 0;
}

void socket::set_remote_addr(LPSOCKADDR remote_addr, int32_t remote_addr_length)
{
	if (sizeof(remote_addr_) > remote_addr_length)
	{
		memcpy(&remote_addr_, remote_addr, remote_addr_length);
	}
}

const char* socket::get_remote_ip()
{
	sockaddr_in* addr = (sockaddr_in*)remote_addr_;
	return inet_ntoa(addr->sin_addr);
}
unsigned short socket::get_remote_port()
{
	sockaddr_in* addr = (sockaddr_in*)remote_addr_;
	return ntohs(addr->sin_port);
}

}
