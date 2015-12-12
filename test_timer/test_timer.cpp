#include <iostream>
#include <functional>
#include <x/io_service.h>
#include <x/deadline_timer.h>

void handle_timer(int32_t error)  
{  
	std::cout << "handle_timer: " << error << std::endl;
}  

void handle_wait(int32_t error,  
				 x::deadline_timer& t,   
				 int32_t& count)  
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

int32_t main()
{
	const int32_t wait_millisec = 1000;
	x::io_service io_service;
	x::deadline_timer t1(io_service);

	int32_t count = 0;
	t1.async_wait(wait_millisec, handle_timer);

	x::deadline_timer t2(io_service);
	t2.async_wait(wait_millisec, std::bind(handle_wait,   
		std::placeholders::_1,  
		std::ref(t2),  
		std::ref(count)));

	io_service.run();

	std::cout << "end" << std::endl;
	std::cin.get();
	return 0;
}

