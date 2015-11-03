#include "message_queue.h"

namespace util
{

message_queue::message_queue(int highCapacity, int mediumCapacity, int lowCapacity)
	: thread_get_(0)
{
	int capacitys[] = {highCapacity, mediumCapacity, lowCapacity};
	for (int i = 0; i < PRIORITY_MAX; ++i)
	{
		queues_[i].datas = new void*[capacitys[i]];
		queues_[i].capacity = capacitys[i];
		queues_[i].size = 0;
		queues_[i].begin = 0;
		queues_[i].end = 0;
		queues_[i].thread_put = 0;
	}
}

message_queue::~message_queue()
{
	delete [] queues_[PRIORITY_HIGH].datas;
	delete [] queues_[PRIORITY_MEDIUM].datas;
	delete [] queues_[PRIORITY_LOW].datas;
}

bool message_queue::enqueue(void* data, long timeout, int priority)
{
	if (priority < PRIORITY_HIGH || priority > PRIORITY_LOW)
	{
		return false;
	}

	single_queue* queue = &queues_[priority];
	unique_lock lock(mutex_);
	while (queue->size == queue->capacity)
	{
		if (timeout == 0)
		{
			return false;
		}

		queue->thread_put++;
		if (!queue->cond_put.timed_wait(mutex_, timeout)) 
		{
			queue->thread_put--;
			return false;
		}
		queue->thread_put--;
	}
	queue->datas[queue->end++] = data;
	if (queue->end == queue->capacity)
	{
		queue->end = 0;
	}
	queue->size++;

	if (thread_get_ > 0)
	{
		cond_get_.notify_one();
	}

	return true;
}

void* message_queue::dequeue(long timeout)
{
	single_queue* queue = NULL;
	unique_lock lock(mutex_);
	while (queue == NULL)
	{
		if (queues_[PRIORITY_HIGH].size > 0)
		{
			queue = &queues_[PRIORITY_HIGH];
		}
		else if (queues_[PRIORITY_MEDIUM].size > 0)
		{
			queue = &queues_[PRIORITY_MEDIUM];
		}
		else if (queues_[PRIORITY_LOW].size > 0)
		{
			queue = &queues_[PRIORITY_LOW];
		}

		if (queue == NULL)
		{
			if (timeout == 0)
			{
				break;
			}

			thread_get_++;
			if (!cond_get_.timed_wait(mutex_, timeout)) 
			{
				thread_get_--;
				break;
			}
			thread_get_--;
		}
		else
		{
			void* data = queue->datas[queue->begin++];
			if (queue->begin == queue->capacity)
			{
				queue->begin = 0;
			}
			queue->size--;

			if (queue->thread_put > 0) 
			{
				queue->cond_put.notify_one();
			}

			return data;
		}
	}
	return NULL;
}

}
