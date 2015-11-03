#include "default_allocator.h"
#include "memory_pool.h"

namespace util
{

namespace
{
	class default_pool_manager
	{
	public:
		static memory_pool& get_pool()
		{
			static memory_pool* pool;

			if (pool == 0)
			{
				unique_lock lock(mutex_);
				if (pool == 0)
				{
					pool = new memory_pool;
					pool->init();
				}
			}
			return *pool;
		}
		static mutex mutex_;
	};

	mutex default_pool_manager::mutex_;
}

void* default_allocator::allocate(unsigned bytes)
{
	return default_pool_manager::get_pool().allocate(bytes);
}

void default_allocator::deallocate(void* ptr)
{
	default_pool_manager::get_pool().deallocate(ptr);
}

}
