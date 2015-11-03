#pragma once

#include "mutex.h"

namespace util
{

// 实现了小内存的内存池
class memory_pool
{
public:
	void init(unsigned align = 8, unsigned max_bytes = 128);
	void deinit();
	void* allocate(unsigned bytes);
	void deallocate(void* ptr);

private:
	void* refill(unsigned bytes);
	char* alloc_chunk(unsigned bytes, unsigned& count);
	void free_chunks();

	union obj
	{
		unsigned size;
		obj* next;
	};

	struct chunk
	{
		unsigned size;
		chunk*	next;
	};
	mutex			mutex_;

	unsigned		align_;
	unsigned		max_bytes_;
	obj* volatile*	free_list_;

	chunk*			chunk_list_;
	unsigned		heap_size_;
	char*			free_start_;
	char*			free_end_;
};

}
