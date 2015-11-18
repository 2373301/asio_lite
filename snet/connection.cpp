#include "connection.h"
#include "agent_impl.h"
#include <functional>

connection::connection(agent_impl& agent_impl,
	asio::io_service& io_service, net_callback* callback)
	: agent_impl_(agent_impl)
	, callback_(callback)
	, socket_(io_service)
	, timer_(io_service)
	, connected_(false)
	, running_(true)
{
}

connection::~connection()
{
}

int connection::open()
{
	return socket_.open();
}

std::string connection::get_remote_ip()
{
	return socket_.get_remote_ip();
}

unsigned short connection::get_remote_port()
{
	return socket_.get_remote_port();
}

void connection::async_connect(const char* ipaddr, unsigned short port, 
				   int timeout_millis, unsigned short local_port)
{
	int ret = socket_.open();
	if (ret != 0)
	{
		agent_impl_.del_connection(id_);
		if (callback_)
		{
			callback_->on_connect(id_, ret);
		}
		return;
	}

	int reuse_addr = 1;
	socket_.set_option(SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr, sizeof(reuse_addr));

	int nodelay = 1;
	socket_.set_option(IPPROTO_TCP, TCP_NODELAY, (char*)&reuse_addr, sizeof(reuse_addr));

	ret = socket_.bind(local_port);
	if (ret != 0)
	{
		agent_impl_.del_connection(id_);
		if (callback_)
		{
			callback_->on_connect(id_, ret);
		}
		return;
	}
	socket_.async_connect(ipaddr, port, 
		std::bind(&connection::handle_connect,
		share_from_this(), std::placeholders::_1));

	timer_.async_wait(timeout_millis, std::bind(&connection::handle_conn_timer,
		share_from_this(), std::placeholders::_1));
}

void connection::handle_conn_timer(int error)
{
	if (!connected_)
	{
		handle_stop();
	}
}

void connection::handle_connect(int error)
{
	connect_finish(error);
	if (!error)
	{
		connected_ = true;
		async_read();
	}
}

void connection::connect_finish(int error)
{
	timer_.cancel();
	if (callback_)
	{
		callback_->on_connect(id_, error);
	}
	if (error)
	{
		handle_stop();
	}
}

void connection::close()
{
	running_ = false;
	socket_.get_io_service().post(
		std::bind(&connection::handle_stop, share_from_this()));
}

void connection::stop_connection()
{
	if (running_)
	{
		handle_stop();
		running_ = false;
		agent_impl_.del_connection(id_);
		if (callback_)
		{
			callback_->on_close(id_);
		}
	}
}

void connection::handle_stop()
{
	socket_.close();
}

void connection::async_read()
{
	socket_.async_read_some(read_data_, 4096,
        std::bind(&connection::handle_read, share_from_this(),
        std::placeholders::_1, std::placeholders::_2));
}

void connection::handle_read(int error, unsigned read_len)
{
	if (!error && read_len > 0)
	{
		if (callback_)
		{
			callback_->on_read(id_, read_data_, read_len);
		}
		async_read();
	}
	else
	{
		stop_connection();
	}
}

void connection::async_write(const void* data, int len)
{
	void* new_data = ::malloc(len);
	memcpy((char*)new_data, data, len);
	socket_.get_io_service().post(
		std::bind(&connection::async_write_inner,
		share_from_this(), new_data, len));
}

void connection::async_write_inner(const void* data, int len)
{
	len_buff lbuff;
	lbuff.len = len;
	lbuff.buff = data;
	bool write_in_progress = !write_qeueu_.empty();
	write_qeueu_.push_back(lbuff);

	if (!write_in_progress)
	{
		socket_.async_write(write_qeueu_.front().buff, write_qeueu_.front().len,
            std::bind(&connection::handle_write,
            share_from_this(), std::placeholders::_1));
	}
}

void connection::handle_write(int error)
{
	if (error)
	{
		while (!write_qeueu_.empty())
		{
			delete [] write_qeueu_.front().buff;
			write_qeueu_.pop_front();
		}
		handle_stop();
		return;
	}
	::free((void*)write_qeueu_.front().buff);
	write_qeueu_.pop_front();
	if (!write_qeueu_.empty())
	{
		socket_.async_write(write_qeueu_.front().buff, write_qeueu_.front().len,
            std::bind(&connection::handle_write,
            share_from_this(), std::placeholders::_1));
	}
}
