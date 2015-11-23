#include "work_thread.h"

void work_thread::start()
{
	work_ = new x::io_service::work(io_service_);
    std::thread temp(std::bind(&x::io_service::run, &io_service_));
    temp.swap(thread_);
}
void work_thread::stop()
{
	delete work_;
	thread_.join();
}

x::io_service& work_thread::get_io_service()
{
	return io_service_;
}

