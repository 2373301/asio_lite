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

	int32_t open();

	x::socket& socket() { return socket_; }
	void set_id(unsigned id){id_ = id;}
	unsigned get_id(){return id_;}
	std::string get_remote_ip();
	unsigned short get_remote_port();

	void async_connect(const char* ipaddr, unsigned short port, 
		int32_t timeout_millis, unsigned short local_port);

	void async_read();
	void async_write(const void* data, int32_t len);
	void close();

private:
	void handle_connect(int32_t error);
	void handle_conn_timer(int32_t error);
	void connect_finish(int32_t error);
	void handle_stop();
	void stop_connection();
	void handle_read(int32_t error, unsigned read_len);
	void async_write_inner(const void* data, int32_t len);
	void handle_write(int32_t error);

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
		int32_t len;
		const void* buff;
	};
	std::deque<len_buff> write_qeueu_;
};

typedef std::shared_ptr<connection> connection_ptr;
