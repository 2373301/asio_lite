#include "memory_pool.h"

namespace util
{

static unsigned round_up(unsigned bytes, unsigned align)
{
	return ((bytes + align - 1) & (~(align - 1)));
}

static unsigned list_index(unsigned bytes, unsigned align)
{
	return ((bytes + align - 1) / align - 1);
}

void memory_pool::init(unsigned align, unsigned max_bytes)
{
	align_ = round_up(align, 8);
	max_bytes_ = round_up(max_bytes, align_);
	unsigned lists_size = max_bytes_ / align_;

	free_list_ = (obj**)::malloc(sizeof(obj*) * lists_size);
	for (unsigned i = 0; i < lists_size; ++i)
	{
		free_list_[i] = 0;
	}

	heap_size_ = 0;
	free_start_ = 0;
	free_end_ = 0;
	chunk_list_ = 0;
}

void memory_pool::deinit()
{
	free_chunks();
	::free((void*)free_list_);
	free_list_ = 0;
}

void* memory_pool::allocate(unsigned bytes)
{
	obj* result;
	obj* volatile* free_list;

	if (bytes == 0)
	{
		return 0;
	}

	if (bytes > max_bytes_)
	{
		result = (obj*)::malloc(bytes + sizeof(unsigned));
	}
	else
	{
		free_list = free_list_ + list_index(bytes, align_);

		unique_lock lock(mutex_);
		result = *free_list;
		if (result == 0)
		{
			result = (obj*)refill(round_up(bytes, align_));
		}
		else
		{
			*free_list = (*free_list)->next;
		}
	}

	if (result == 0)
	{
		return 0;
	}

	result->size = bytes;
	return (char*)result + sizeof(unsigned);
}

void memory_pool::deallocate(void* ptr)
{
	obj* volatile* free_list;
	obj* free_obj;

	if (ptr == 0)
	{
		return;
	}

	free_obj = (obj*)((char*)ptr - sizeof(unsigned));

	if ((free_obj->size) > (max_bytes_))
	{
		::free(free_obj);
		return;
	}

	free_list = free_list_ + list_index(free_obj->size, align_);

	unique_lock lock(mutex_);
	free_obj->next = *free_list;
	*free_list = free_obj;
}

void* memory_pool::refill(unsigned bytes)
{
	unsigned count = 20;
	obj* volatile* free_list;
	obj* result;
	obj* current;
	obj* next;

	char* new_chunk = alloc_chunk(bytes, count);

	if (count == 0)
	{
		return 0;
	}
	if (count == 1)
	{
		return new_chunk;
	}

	free_list = free_list_ + list_index(bytes, align_);
	result = (obj*)new_chunk;
	*free_list = next = (obj*)(new_chunk + bytes + sizeof(unsigned));
	for (unsigned i = 1; ; ++i)
	{
		current = next;
		next = (obj*)((char*)next + bytes + sizeof(unsigned));
		if (count - 1 == i)
		{
			current->next = 0;
			break;
		}
		else
		{
			current->next = next;
		}
	}

	return result;
}

char* memory_pool::alloc_chunk(unsigned bytes, unsigned& count)
{
	char* result;
	unsigned real_bytes = bytes + sizeof(unsigned);
	unsigned total_bytes = real_bytes * count;
	unsigned left_bytes = (unsigned int)(free_end_ - free_start_);

	if (left_bytes >= total_bytes)
	{
		result = free_start_;
		free_start_ += total_bytes;
		return result;
	}
	else if (left_bytes >= real_bytes)
	{
		count = left_bytes / real_bytes;
		result = free_start_;
		free_start_ += real_bytes * count;
		return result;
	}
	else
	{
		if (left_bytes >= sizeof(unsigned) + align_)
		{
			obj* volatile* freeList = free_list_ + 
				(left_bytes - sizeof(unsigned)) / align_ - 1;
			((obj*)free_start_)->next = *freeList;
			*freeList = (obj*)free_start_;
		}

		unsigned bytes_alloc = total_bytes * 2 + round_up(heap_size_ >> 4, align_);
		char* new_chunk = (char*)::malloc(bytes_alloc + sizeof(chunk));

		if (new_chunk == 0)
		{
			obj*volatile* free_list;
			for (unsigned i = bytes; i <= max_bytes_; i += align_)
			{
				free_list = free_list_ + list_index(i, align_);
				if (*free_list != 0)
				{
					obj* current = *free_list;
					*free_list = (*free_list)->next;
					free_start_ = (char*)current;
					free_end_ = free_start_ + i;
					return alloc_chunk(bytes, count);
				}
			}
			count = 0;
			return 0;
		}

		((chunk*)new_chunk)->next = chunk_list_;
		((chunk*)new_chunk)->size = bytes_alloc;
		chunk_list_ = (chunk*)new_chunk;

		heap_size_ += bytes_alloc;
		free_start_ = new_chunk + sizeof(chunk);
		free_end_ = free_start_ + bytes_alloc;

		return alloc_chunk(bytes, count);
	}
}

void memory_pool::free_chunks()
{
	if (chunk_list_ != 0)
	{
		chunk* next = chunk_list_;
		chunk* current;

		while (next != 0)
		{
			current = next;
			next = next->next;
			::free(current);
		}
		chunk_list_ = 0;
	}
}

}
