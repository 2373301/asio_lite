#include "agent_impl.h"
#include "acceptor.h"

net_agent* create_net_agent()
{
	return agent_impl::create();
}

agent_impl* agent_impl::create()
{
	return new agent_impl;
}

agent_impl::agent_impl()
	: threads_(NULL)
	, thread_num_(0)
	, thread_index_(0)
	, running_(false)
	, conn_id_(0)
{
}

agent_impl::~agent_impl()
{
}

void agent_impl::init(net_callback* callback, unsigned thread_num)
{
	if (running_)
	{
		return;
	}
	net_callback_ = callback;
	thread_num_ = thread_num;
	threads_ = new work_thread[thread_num_];
	for (unsigned i = 0; i < thread_num_; ++i)
	{
		threads_[i].start();
	}
	running_ = true;
}

void agent_impl::release()
{
	if (!running_)
	{
		return;
	}

	{
		std::unique_lock<std::mutex> ul(mutex_conn_);
		for (auto iter = conn_map_.begin();
			iter != conn_map_.end(); ++iter)
		{
			iter->second->close();
		}
		conn_map_.clear();
	}
	
	stop_server();
	for (unsigned i = 0; i < thread_num_; ++i)
	{
		threads_[i].stop();
	}
	delete [] threads_;
	running_ = false;
	delete this;
}

x::io_service& agent_impl::get_io_service()
{
	int index = (thread_index_++) % thread_num_;
	return threads_[index].get_io_service();
}

int agent_impl::start_server(unsigned short port)
{
	stop_server();
	acceptor_.reset(new acceptor(*this, get_io_service()));
	int ret = acceptor_->start(port, net_callback_);
	if (ret != 0)
	{
		acceptor_.reset();
	}
	return 0;
}

void agent_impl::stop_server()
{
	if (acceptor_.get())
	{
		acceptor_->stop();
		acceptor_.reset();
	}
}

void agent_impl::add_connection(connection_ptr conn)
{
	std::unique_lock<std::mutex> ul(mutex_conn_);
	unsigned id = 0;
	do 
	{
		id = conn_id_++;
	} while (conn_map_.find(id) != conn_map_.end());

	conn->set_id(id);
	conn_map_[id] = conn;
}

connection_ptr agent_impl::get_connetion(unsigned id)
{
	std::unique_lock<std::mutex> ul(mutex_conn_);
	connection_ptr conn;
	auto iter = conn_map_.find(id);
	if (iter != conn_map_.end())
	{
		conn = iter->second;
	}
	return conn;
}

void agent_impl::del_connection(unsigned id)
{
	std::unique_lock<std::mutex> ul(mutex_conn_);
	conn_map_.erase(id);
}

unsigned agent_impl::connect(const char* ipaddr, unsigned short port, 
				 int timeout_millis, unsigned short local_port)
{
	connection_ptr new_conn(new connection(*this, get_io_service(), net_callback_));
	add_connection(new_conn);
	new_conn->async_connect(ipaddr, port, timeout_millis, local_port);
	return new_conn->get_id();
}

int agent_impl::send_data(unsigned id, const void* data, int len)
{
	connection_ptr conn = get_connetion(id);
	if (conn.get() != NULL)
	{
		conn->async_write(data, len);
		return 0;
	}
	return -1;
}

void agent_impl::close(unsigned id)
{
	connection_ptr conn = get_connetion(id);
	del_connection(id);
	if (conn.get() != NULL)
	{
		conn->close();
	}
}
