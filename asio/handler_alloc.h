#pragma once

namespace asio
{

class handler_alloc
{
public:
	handler_alloc()
		: in_use_(false)
	{
	}

	void* allocate(unsigned int size)
	{
		if (!in_use_ && size < sizeof(storage_))
		{
			in_use_ = true;
			return &storage_;
		}
		else
		{
			return ::malloc(size);
		}
	}

	void deallocate(void* ptr)
	{
		if (ptr == storage_)
		{
			in_use_ = false;
		}
		else
		{
			::free(ptr);
		}
	}

private:
	char storage_[128];
	bool in_use_;
};

template <typename Handler>
class alloc_handler
{
public:
	alloc_handler(handler_alloc& a, Handler h)
		: allocator_(a), handler_(h)
	{
	}

	void operator()()
	{
		handler_();
	}

	template <typename A1>
	void operator()(A1 a1)
	{
		handler_(a1);
	}

	template <typename A1, typename A2>
	void operator()(A1 a1, A2 a2)
	{
		handler_(a1, a2);
	}

	friend void* asio_handler_alloc(unsigned int size,
		alloc_handler<Handler>* handler)
	{
		return handler->allocator_.allocate(size);
	}

	friend void asio_handler_dealloc(void* ptr, alloc_handler<Handler>* handler)
	{
		handler->allocator_.deallocate(ptr);
	}

private:
	handler_alloc& allocator_;
	Handler handler_;
};

template <typename Handler>
inline alloc_handler<Handler> make_alloc_handler(
	handler_alloc& a, Handler h)
{
	return alloc_handler<Handler>(a, h);
}

}
