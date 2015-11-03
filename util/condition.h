#pragma once

#include "noncopyable.h"
#include "mutex.h"

namespace util
{

class condition : private noncopyable
{
public:
	condition();
	~condition();

	void wait(mutex& mutex);
	bool timed_wait(mutex& mutex, unsigned milliscd);
	void notify_one();
	void notify_all();

private:
#if defined (_MSC_VER)
	CRITICAL_SECTION csection_;
	HANDLE gate_;
	HANDLE queue_;
	unsigned gone_;
	unsigned long blocked_;
	unsigned waiting_;
#elif defined(__GNUC__)
	pthread_cond_t cond_;
#endif
};

/*
typedef struct
{
	char buf[BSIZE];
	int occupied;
	int nextin;
	int nextout;
	pthread_mutex_t mutex;
	pthread_cond_t more;
	pthread_cond_t less;
} buffer_t;

buffer_t buffer;

void producer(buffer_t *b, char item)
{
	pthread_mutex_lock(&b->mutex);

	while (b->occupied >= BSIZE)
		pthread_cond_wait(&b->less, &b->mutex);

	assert(b->occupied < BSIZE);

	b->buf[b->nextin++] = item;
	b->nextin %= BSIZE;
	b->occupied++;

	pthread_cond_signal(&b->more);
	pthread_mutex_unlock(&b->mutex);
}

char consumer(buffer_t *b)
{
	char item;
	pthread_mutex_lock(&b->mutex);
	while (b->occupied <= 0)
		pthread_cond_wait(&b->more, &b->mutex);

	assert(b->occupied > 0);

	item = b->buf[b->nextout++];
	b->nextout %= BSIZE;
	b->occupied--;

	pthread_cond_signal(&b->less);
	pthread_mutex_unlock(&b->mutex);

	return (item);
}
*/

}
