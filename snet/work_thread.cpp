#include "work_thread.h"

void work_thread::start()
{
	work_ = new asio::io_service::work(io_service_);
	thread_.start(&io_service_, &asio::io_service::run);
}
void work_thread::stop()
{
	delete work_;
	thread_.join();
}

asio::io_service& work_thread::get_io_service()
{
	return io_service_;
}

