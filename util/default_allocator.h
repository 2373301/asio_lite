#pragma once

#include "config.h"

namespace util
{

// 默认内存池，init(8, 128)
class default_allocator
{
public:
	static void* allocate(unsigned bytes);
	static void deallocate(void* ptr);
};

class pool_objcet
{
public:
	void* operator new(unsigned len)
	{
		return default_allocator::allocate(len);
	}
	void operator delete(void* ptr)
	{
		default_allocator::deallocate(ptr);
	}
	void* operator new[](unsigned len)
	{
		return default_allocator::allocate(len);
	}
	void operator delete[](void* ptr)
	{
		default_allocator::deallocate(ptr);
	}
};

// 实现了stl的内存分配器
template <typename Type, typename Pool = default_allocator>
	class stl_allocator
{
public:
	typedef Type value_type;
	typedef Pool pool_type;

	typedef value_type* pointer;
	typedef value_type& reference;

	typedef const value_type* const_pointer;
	typedef const value_type& const_reference;

	typedef std::size_t size_type;
	typedef ptrdiff_t difference_type;

	template <typename Other> 
		struct rebind 
	{
		typedef stl_allocator<Other, pool_type> other;
	};

	stl_allocator() 
	{
	}

	stl_allocator(const stl_allocator<value_type, pool_type>&)
	{
	}

	template <typename Other>
		stl_allocator(const stl_allocator<Other, pool_type>&) 
	{
	}

	template<class Other>
		stl_allocator<value_type, pool_type>& operator=(const stl_allocator<Other, pool_type>&)
	{
		return (*this);
	}

	pointer address(reference val) const 
	{
		return &val; 
	}

	const_pointer address(const_reference val) const 
	{
		return &val; 
	}

	pointer allocate(size_type count, const void* = 0)
	{
		pointer ptr = (pointer)
			pool_type::allocate((unsigned int)sizeof(value_type) * count);
		if (ptr == 0)
		{
			throw std::exception("allocate error in stl_allocator");
		}
		return ptr;
	}

	void deallocate(pointer ptr, size_type)
	{
		pool_type::deallocate(ptr);
	}

	void construct(pointer ptr, const value_type& val) 
	{ 
		new ((void*)ptr)value_type(val); 
	}

	void destroy(pointer ptr) 
	{
		ptr->~value_type(); 
	}

	size_type max_size() const  
	{
		size_type count = (size_type)(-1) / sizeof (value_type);
		return (0 < count ? count : 1);
	}
};

}