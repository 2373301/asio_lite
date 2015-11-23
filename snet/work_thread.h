#pragma once

#include <x/io_service.h>
#include <thread>

class work_thread
{
public:
	void start();
	void stop();

	x::io_service& get_io_service();

private:
	x::io_service io_service_;
	x::io_service::work* work_;
	std::thread thread_;
};

