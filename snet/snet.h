#pragma once
#include <stdint.h>

struct net_callback
{
	virtual void on_connect(unsigned id, int32_t result){}
	virtual void on_accept(unsigned id, const char* ipaddr, unsigned short port){}
	virtual void on_close(unsigned id) = 0;
	virtual void on_read(unsigned id, const void* data, unsigned len) = 0;
};

struct net_agent
{
	virtual void init(net_callback* callback, unsigned thread_num =3) = 0;
	virtual void release() = 0;

	virtual int32_t start_server(unsigned short port) = 0;
	virtual void stop_server() = 0;

	virtual unsigned connect(const char* ipaddr, unsigned short port, 
		                      int32_t timeout_millis, unsigned short local_port = 0) = 0;
	virtual int32_t send_data(unsigned id, const void* data, int32_t len) = 0;
	virtual void close(unsigned id) = 0;
	virtual unsigned getids() = 0;

	virtual ~net_agent(){}
};

net_agent* create_net_agent();
