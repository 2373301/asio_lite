#include <iostream>
#include <string>
#include <set>
#include <functional>
#define WIN32_LEAN_AND_MEAN
#include <snet/snet.h>
#include <x/io_service.h>
#include <x/deadline_timer.h>

using namespace std;

class  server : public net_callback
{
public:
	server() : timer_stop_(io_service_)
	{
		
	}
	void start()
	{
		agent_ = create_net_agent();
		agent_->init(this, 5);
		int ret = agent_->start_server(8000);
		if (ret != 0)
		{
			std::unique_lock<std::mutex> ul(mutex_);
			cout << "start_server failed: " << ret << endl;
		}
		timer_stop_.async_wait(12000, std::bind(&server::handle_stop_timer, this, std::placeholders::_1));
	}
	void handle_stop_timer(int error)
	{
		std::unique_lock<std::mutex> ul(mutex_);
		cout << "handle_stop_timer: ids(" << conn_ids_.size() << "," << agent_->getids() << ")" << endl;
		int count  = 0;
		while (count++ < 20 && !conn_ids_.empty())
		{
			auto iter = conn_ids_.begin();
			agent_->close(*iter);
			conn_ids_.erase(iter);
		}
		timer_stop_.async_wait(12000, std::bind(&server::handle_stop_timer, this, std::placeholders::_1));
	}
	virtual void on_accept(unsigned id, const char* ipaddr, unsigned short port)
	{
		std::unique_lock<std::mutex> ul(mutex_);
		conn_ids_.insert(id);
		//cout << "on_accept: id=" << id << ", ip=" << ipaddr << ", port=" << port << endl;
	}
	virtual void on_close(unsigned id)
	{
		std::unique_lock<std::mutex> ul(mutex_);
		conn_ids_.erase(id);
		//cout << "on_close: id=" << id << endl;
	}
	virtual void on_read(unsigned id, const void* data, unsigned len)
	{
		agent_->send_data(id, data, len);
		//cout << "on_read: id=" << id << ", len=" << len << endl;
		//cout << string((char*)data, len) << endl;
	}
	void run()
	{
		io_service_.run();
	}

private:
	net_agent* agent_;
	std::mutex mutex_;
	set<unsigned> conn_ids_;
	x::io_service io_service_;
	x::deadline_timer timer_stop_;
};

class client : public net_callback
{
public:
	client() : timer_send_(io_service_), timer_stop_(io_service_)
	{
		x::io_service::work* work = new x::io_service::work(io_service_);
		agent_ = create_net_agent();
		agent_->init(this, 5);
	}
	void start()
	{
		{
			std::unique_lock<std::mutex> ul(mutex_);
			cout << "connect..." << endl;
		}
		for (int i = 0; i < 300; ++i)
		{
			unsigned id = agent_->connect("127.0.0.1", 8000, 5000, 0);
			
		}
 		count_ = 0;
		timer_send_.async_wait(500, std::bind(&client::handle_send_timer, this, std::placeholders::_1));
	}

	void handle_send_timer(int error)
	{
		{
			std::unique_lock<std::mutex> ul(mutex_);
			cout << "handle_send_timer, sned(" << count_++ << "), ids(" << conn_ids_.size() << "," << agent_->getids() << ")" << endl;
			for (auto iter = conn_ids_.begin(); iter != conn_ids_.end(); ++iter)
			{
				agent_->send_data(*iter, buff, 1024);
			}
		}

		if (count_ < 20)
		{
			timer_send_.async_wait(500, std::bind(&client::handle_send_timer, this, std::placeholders::_1));
		}
		else
		{
			std::unique_lock<std::mutex> ul(mutex_);
			for (auto iter = conn_ids_.begin(); iter != conn_ids_.end(); ++iter)
			{
				agent_->close(*iter);
			}
			conn_ids_.clear();
			Sleep(1000);
			start();
		}
	}

	virtual void on_connect(unsigned id, int result)
	{
		std::unique_lock<std::mutex> ul(mutex_);
		if (result == 0)
		{
			conn_ids_.insert(id);
		}
		else
		{
			cout << "on_connect: id=" << id << ", result=" << result << endl;
		}
	}
	virtual void on_close(unsigned id)
	{
		std::unique_lock<std::mutex> ul(mutex_);
		conn_ids_.erase(id);
		cout << "on_close: id=" << id << endl;
	}
	virtual void on_read(unsigned id, const void* data, unsigned len)
	{
		//cout << "on_read: id=" << id << ", len=" << len << endl;
		//cout << string((char*)data, len) << endl;
	}

	void run()
	{
		io_service_.run();
	}

private:
	int count_;
	net_agent* agent_;
	std::mutex mutex_;
	set<unsigned> conn_ids_;
	x::io_service io_service_;
	x::deadline_timer timer_send_;
	x::deadline_timer timer_stop_;
	char buff[1024];
};



int main(int argc, char** argv)
{
	if (argc == 2)
	{
		cout << "server..." << endl;
		server* s = new server;
		s->start();
		s->run();
	}
	else
	{
		client c;
		c.start();
		c.run();
	}

	cout << "waiting..." << endl;
	cin.get();
	return 0;
}
