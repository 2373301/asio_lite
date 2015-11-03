#include <iostream>
#include <functional>
#include <asio/io_service.h>
#include <asio/deadline_timer.h>

void handle_timer(int error)  
{  
	std::cout << "handle_timer: " << error << std::endl;
}  

void handle_wait(int error,  
				 asio::deadline_timer& t,   
				 int& count)  
{  
	if(!error)  
	{  
		std::cout << "handle_wait: " << count << std::endl;  

		t.async_wait(1000, std::bind(handle_wait,   
			std::placeholders::_1,  
			std::ref(t),  
			std::ref(count)));  
		if (count++ == 10)  
		{  
			t.cancel();  
		}  
	}
	else
	{
		std::cout << "timer canceled" << std::endl; 
	}
}  

int main()
{
	const int wait_millisec = 1000;
	asio::io_service io_service;
	asio::deadline_timer t1(io_service);

	int count = 0;
	t1.async_wait(wait_millisec, handle_timer);

	asio::deadline_timer t2(io_service);
	t2.async_wait(wait_millisec, std::bind(handle_wait,   
		std::placeholders::_1,  
		std::ref(t2),  
		std::ref(count)));

	io_service.run();

	std::cout << "end" << std::endl;
	std::cin.get();
	return 0;
}

