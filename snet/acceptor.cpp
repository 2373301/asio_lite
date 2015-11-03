#include "acceptor.h"
#include "connection.h"
#include "agent_impl.h"

acceptor::acceptor(agent_impl& agent_impl, asio::io_service& io_service)
	: agent_impl_(agent_impl)
	, socket_(io_service)
	, running_(false)
{
}

acceptor::~acceptor(void)
{
}

int acceptor::start(unsigned short port, net_callback* callback)
{
	int ret = socket_.open();
	if (ret != 0)
	{
		return ret;
	}

	int reuse_addr = 1;
	socket_.set_option(SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr, sizeof(reuse_addr));

	int nodelay = 1;
	socket_.set_option(IPPROTO_TCP, TCP_NODELAY, (char*)&reuse_addr, sizeof(reuse_addr));

	ret = socket_.bind(port);
	if (ret != 0)
	{
		return ret;
	}
	ret = socket_.listen();
	if (ret != 0)
	{
		return ret;
	}

	callback_ = callback;
	running_ = true;
	start_accept();

	return 0;
}

void acceptor::stop()
{
	socket_.get_io_service().post(std::bind(&acceptor::handle_close, share_from_this()));
}

void acceptor::start_accept()
{
	connection_ptr new_conn(new connection(agent_impl_,
		agent_impl_.get_io_service(), callback_));
	new_conn->open();
	socket_.async_accept(new_conn->socket(),
		asio::make_alloc_handler(alloc_, std::bind(&acceptor::handle_accept,
		share_from_this(), new_conn, std::placeholders::_1)));
}

void acceptor::handle_accept(connection_ptr new_conn, int error)
{
	if (!error)
	{
		agent_impl_.add_connection(new_conn);
		if (callback_)
		{
			callback_->on_accept(new_conn->get_id(),
				new_conn->get_remote_ip().c_str(),
				new_conn->get_remote_port());
		}
		new_conn->async_read();

		start_accept();
	}
	else if (running_)
	{
		socket_.async_accept(new_conn->socket(),
			asio::make_alloc_handler(alloc_, std::bind(&acceptor::handle_accept,
			share_from_this(), new_conn, std::placeholders::_1)));
	}
	else
	{
		// stop
	}
}

void acceptor::handle_close()
{
	socket_.close();
}