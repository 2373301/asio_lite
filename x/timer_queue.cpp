#include "timer_queue.h"
#include "deadline_timer.h"
#include "io_service.h"

namespace x
{

timer_queue::timer_queue() : timers_(0), heap_()
{
}

timer_queue::~timer_queue()
{
}

bool timer_queue::enqueue_timer(long long time, per_timer_data& timer, io_operation* op)
{
	// Enqueue the timer object.
	if (timer.prev_ == 0 && &timer != timers_)
	{

		// Put the new timer at the correct position in the heap. This is done
		// first since push_back() can throw due to allocation failure.
		timer.heap_index_ = heap_.size();
		heap_entry entry = { time, &timer };
		heap_.push_back(entry);
		up_heap(heap_.size() - 1);

		// Insert the new timer into the linked list of active timers.
		timer.next_ = timers_;
		timer.prev_ = 0;
		if (timers_)
			timers_->prev_ = &timer;
		timers_ = &timer;
	}

	// Enqueue the individual timer operation.

	if (timer.op_ != 0)
		timer.op_->destroy();
	timer.op_ = op;

	// Interrupt reactor only if newly added timer is first to expire.
	return timer.heap_index_ == 0;
}

// Whether there are no timers in the queue.
bool timer_queue::empty() const
{
	return timers_ == 0;
}

unsigned int timer_queue::cancel_timer(per_timer_data& timer, op_queue<io_operation>& ops)
{
	unsigned int num_cancelled = 0;
	if (timer.prev_ != 0 || &timer == timers_)
	{
		timer.op_->error_ = -1;
		ops.push(timer.op_);
		timer.op_ = 0;
		++num_cancelled;
		remove_timer(timer);
	}
	return num_cancelled;
}

long timer_queue::wait_duration_msec(long max_duration) const
{
	if (heap_.empty())
		return max_duration;

	return long(heap_[0].time - io_service::get_time());
}

void timer_queue::remove_timer(per_timer_data& timer)
{
	// Remove the timer from the heap.
	unsigned int index = timer.heap_index_;
	if (!heap_.empty() && index < heap_.size())
	{
		if (index == heap_.size() - 1)
		{
			heap_.pop_back();
		}
		else
		{
			swap_heap(index, heap_.size() - 1);
			heap_.pop_back();
			unsigned int parent = (index - 1) / 2;
			if (index > 0 && heap_[index].time < heap_[parent].time)
				up_heap(index);
			else
				down_heap(index);
		}
	}

	// Remove the timer from the linked list of active timers.
	if (timers_ == &timer)
		timers_ = timer.next_;
	if (timer.prev_)
		timer.prev_->next_ = timer.next_;
	if (timer.next_)
		timer.next_->prev_= timer.prev_;
	timer.next_ = 0;
	timer.prev_ = 0;
}

void timer_queue::get_ready_timers(op_queue<io_operation>& ops)
{
	if (!heap_.empty())
	{
		long long now = io_service::get_time();
		while (!heap_.empty() && now >= heap_[0].time)
		{
			per_timer_data* timer = heap_[0].timer;
			ops.push(timer->op_);
			timer->op_ = 0;
			remove_timer(*timer);
		}
	}
}

void timer_queue::get_all_timers(op_queue<io_operation>& ops)
{
	while (timers_)
	{
		per_timer_data* timer = timers_;
		timers_ = timers_->next_;
		ops.push(timer->op_);
		timer->op_ = 0;
		timer->next_ = 0;
		timer->prev_ = 0;
	}

	heap_.clear();
}

unsigned int timer_queue::get_size()
{
	return heap_.size();
}

void timer_queue::up_heap(unsigned int index)
{
	unsigned int parent = (index - 1) / 2;
	while (index > 0
		&& heap_[index].time < heap_[parent].time)
	{
		swap_heap(index, parent);
		index = parent;
		parent = (index - 1) / 2;
	}
}

void timer_queue::down_heap(unsigned int index)
{
	unsigned int child = index * 2 + 1;
	while (child < heap_.size())
	{
		unsigned int min_child = (child + 1 == heap_.size()
			|| heap_[child].time < heap_[child + 1].time)
			? child : child + 1;
		if (heap_[index].time < heap_[min_child].time)
			break;
		swap_heap(index, min_child);
		index = min_child;
		child = index * 2 + 1;
	}
}

void timer_queue::swap_heap(unsigned int index1, unsigned int index2)
{
	heap_entry tmp = heap_[index1];
	heap_[index1] = heap_[index2];
	heap_[index2] = tmp;
	heap_[index1].timer->heap_index_ = index1;
	heap_[index2].timer->heap_index_ = index2;
}

}
