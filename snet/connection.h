#pragma once

#include <string>
#include <deque>
#include <memory>
#include <x/io_service.h>
#include <x/deadline_timer.h>

#include "snet.h"

class agent_impl;

class connection : public std::enable_shared_from_this<connection>
{
public:
	connection(agent_impl& agent_impl, 
		x::io_service& io_service, net_callback* callback);
	~connection();

	int open();

	x::socket& socket() { return socket_; }
	void set_id(unsigned id){id_ = id;}
	unsigned get_id(){return id_;}
	std::string get_remote_ip();
	unsigned short get_remote_port();

	void async_connect(const char* ipaddr, unsigned short port, 
		int timeout_millis, unsigned short local_port);

	void async_read();
	void async_write(const void* data, int len);
	void close();

private:
	void handle_connect(int error);
	void handle_conn_timer(int error);
	void connect_finish(int error);
	void handle_stop();
	void stop_connection();
	void handle_read(int error, unsigned read_len);
	void async_write_inner(const void* data, int len);
	void handle_write(int error);

	agent_impl& agent_impl_;
	net_callback* callback_;
	x::socket socket_;
	unsigned id_;
	x::deadline_timer timer_;
	bool connected_;
	bool running_;
	char read_data_[4096];

	struct len_buff
	{
		int len;
		const void* buff;
	};
	std::deque<len_buff> write_qeueu_;
};

typedef std::shared_ptr<connection> connection_ptr;
