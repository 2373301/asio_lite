#pragma once

#define WIN32_LEAN_AND_MEAN
#include <functional>
#include <map>
#include "snet.h"
#include "work_thread.h"
#include "connection.h"
#include "acceptor.h"

class acceptor;

class agent_impl : public net_agent
{
public:
	static agent_impl* create();
	virtual ~agent_impl();

	x::io_service& get_io_service();
	void add_connection(connection_ptr conn);
	connection_ptr get_connetion(unsigned id);
	void del_connection(unsigned id);

protected:
	virtual void init(net_callback* callback, unsigned thread_num = 3);
	virtual void release();

	virtual int32_t start_server(unsigned short port);
	virtual void stop_server();

	virtual unsigned connect(const char* ipaddr, unsigned short port, 
		int32_t timeout_millis, unsigned short local_port = 0);
	virtual int32_t send_data(unsigned id, const void* data, int32_t len);
	virtual void close(unsigned id);
	virtual unsigned getids()
	{
		std::unique_lock<std::mutex> ul(mutex_conn_);
		return conn_map_.size();
	}

private:
	agent_impl();

	work_thread* threads_;
	unsigned thread_num_;
	unsigned thread_index_;
	bool running_;

	net_callback* net_callback_;
	acceptor_ptr acceptor_;

	std::mutex mutex_conn_;
	std::map<unsigned, connection_ptr> conn_map_;
	unsigned conn_id_;
};

