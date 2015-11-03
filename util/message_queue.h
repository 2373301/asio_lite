#pragma once

#include "condition.h"

namespace util
{

class message_queue
{
public:
	enum priority{PRIORITY_HIGH, PRIORITY_MEDIUM, PRIORITY_LOW, PRIORITY_MAX};

	message_queue(int highCapacity = 100, int mediumCapacity = 10000, int lowCapacity = 100);
	~message_queue();

	bool enqueue(void* data, long timeout, int priority);
	void* dequeue(long timeout);

private:
	struct single_queue
	{
		condition cond_put;
		void** datas;
		int capacity;
		int size;
		int begin;
		int end;
		int thread_put;
	};
	mutex mutex_;
	condition cond_get_;
	single_queue queues_[PRIORITY_MAX];
	int thread_get_;
};

}

