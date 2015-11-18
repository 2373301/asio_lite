#pragma once

#include <util/shared_ptr.h>
#include "connection.h"


class agent_impl;

class acceptor : public util::enable_share_from_this<acceptor>
{
public:
	acceptor(agent_impl& agent_impl, asio::io_service& io_service);
	~acceptor();

	int start(unsigned short port, net_callback* callback);
	void stop();

private:
	void start_accept();
	void handle_accept(connection_ptr new_conn, int error);
	void handle_close();

private:
	agent_impl& agent_impl_;
	asio::socket socket_;
	net_callback* callback_;
	bool running_;
};

typedef util::shared_ptr<acceptor> acceptor_ptr;
