#pragma once

#include <asio/io_service.h>

class work_thread
{
public:
	void start();
	void stop();

	asio::io_service& get_io_service();

private:
	asio::io_service io_service_;
	asio::io_service::work* work_;
	util::thread thread_;
};

