#pragma once

#include <vector>
#include "noncopyable.h"
#include "op_queue.h"
#include "io_operation.h"

namespace x
{

class per_timer_data
{
public:
	per_timer_data() 
        : op_(0), next_(0), prev_(0)
    {}

private:
	friend class timer_queue;

	io_operation* op_;
	unsigned int heap_index_;
	per_timer_data* next_;
	per_timer_data* prev_;
};

class timer_queue 
    : private x::noncopyable
{
public:
	timer_queue();
	~timer_queue();

	bool enqueue_timer(long long time, per_timer_data& timer, io_operation* op);
	bool empty() const;
	unsigned int cancel_timer(per_timer_data& timer, op_queue<io_operation>& ops);
	long wait_duration_msec(long max_duration) const;
	void get_ready_timers(op_queue<io_operation>& ops);
	void get_all_timers(op_queue<io_operation>& ops);
	unsigned int get_size();

private:
	void remove_timer(per_timer_data& timer);
	void up_heap(unsigned int index);
	void down_heap(unsigned int index);
	void swap_heap(unsigned int index1, unsigned int index2);

	struct heap_entry
	{
		long long time;
		per_timer_data* timer;
	};

	// The head of a linked list of all active timers.
	per_timer_data* timers_;
	std::vector<heap_entry> heap_;
};

}
