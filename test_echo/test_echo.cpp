#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <functional>
#include <x/io_service.h>
#include <x/deadline_timer.h>

int32_t str_char(char* str, int32_t len, char c);

class session
{
public:
	session(x::io_service& io_service)
		: socket_(io_service), len_(0)
	{
		socket_.open();
	}

	x::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		strcpy(data_, "hello\r\n");
		socket_.async_write(data_, strlen(data_),
			std::bind(&session::handle_write, this, std::placeholders::_1));
	}

	void handle_read(int32_t error, size_t bytes_transferred)
	{
		if (!error)
		{
			len_ += bytes_transferred;
			int32_t end_pos = str_char(data_, len_, '\n');
			if (end_pos != -1 || len_ >= max_length)
			{
				socket_.async_write(data_, len_,
					std::bind(&session::handle_write, this, std::placeholders::_1));
			}
			else
			{
				socket_.async_read_some(data_ + len_, max_length - len_,
					std::bind(&session::handle_read, this,
					std::placeholders::_1,
					std::placeholders::_2));
			}
		}
		else
		{
			delete this;
		}
	}

	void handle_write(int32_t error)
	{
		len_ = 0;
		if (!error)
		{
			socket_.async_read_some(data_, max_length,
				std::bind(&session::handle_read, this,
				std::placeholders::_1, std::placeholders::_2));
		}
		else
		{
			delete this;
		}
	}

private:
	x::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
	int32_t len_;
};

class server
{
public:
	server(x::io_service& io_service, short port)
		: io_service_(io_service), acceptor_(io_service)
	{
		acceptor_.open();
		acceptor_.bind(port);
		acceptor_.listen();

		session* new_session = new session(io_service_);
		acceptor_.async_accept(new_session->socket(),
			std::bind(&server::handle_accept, this, new_session,
			std::placeholders::_1));
	}

	void handle_accept(session* new_session, int32_t error)
	{
		if (!error)
		{
			new_session->start();
			new_session = new session(io_service_);
			acceptor_.async_accept(new_session->socket(),
				std::bind(&server::handle_accept, this, new_session,
				std::placeholders::_1));
		}
		else
		{
			delete new_session;
		}
	}

private:
	x::io_service& io_service_;
	x::socket acceptor_;
};

int32_t main()
{
	x::io_service io_service;
	server s(io_service, 8000);

	const int32_t THREAD_NUM = 5;
	std::thread threads[THREAD_NUM];
	for (std::size_t i = 0; i < THREAD_NUM; ++i)
	{   
        std::thread temp( &x::io_service::run, &io_service);
        temp.swap(threads[i]);
	}

	for (std::size_t i = 0; i < THREAD_NUM; ++i)
		threads[i].join();

	std::cin.get();
	return 0;
}

int32_t str_char(char* str, int32_t len, char c)
{
	for (int32_t i=0; i<len; ++i)
	{
		if (str[i] == c)
			return i;
	}
	return -1;
}