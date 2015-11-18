#pragma once

#include <util/thread.h>
#include <util/mutex.h>
#include <thread>
#include <mutex>
#include "timer_queue.h"
#include "socket.h"

namespace asio
{

class deadline_timer;
class io_service
{
public:
    class work;

public:
    io_service();
    ~io_service();

    int register_handle(socket& s);

    template<typename Handle>
    void post(Handle h)
    {
        typedef io_operation_post<Handle> op;
        op* p = new op(h);
        post_immediate_completion(p);
    }

    template<typename Handle>
    void add_async_timer(deadline_timer& timer, long millisec, Handle h)
    {
        typedef io_operation1<Handle> op;
        op* p = new op(h);

        if (::InterlockedExchangeAdd(&shutdown_, 0) != 0)
        {
            post_immediate_completion(p);
            return;
        }

        std::unique_lock<std::mutex> lock(dispatch_mutex_);
        if (timer_event_ == NULL)
        {
            timer_event_ = ::CreateWaitableTimer(NULL, FALSE, NULL);
            std::thread t(&io_service::timer_thread, this);
            timer_thread_.swap(t);
        }
        bool earliest = timer_queue_.enqueue_timer(
            get_time() + millisec, timer.per_timer_data_, p);

        work_started();
        if (earliest)
        {
            update_timeout();
        }
    }

    unsigned int cancel_timer(deadline_timer& timer);
    void work_started();
    void work_finished();
    void run();

    void stop();
    void reset();

    static long long get_time();
    static accept_ex_fn get_accept_ex(socket& s);
    static accept_ex_socketaddrs_fn get_accept_ex_socketaddrs(socket& s);
    static connect_ex_fn get_connect_ex(socket& s);

private:
    void shutdown_service();
    void update_timeout();

    void on_pending(io_operation* op);
    void on_completion(io_operation* op, DWORD last_error, DWORD bytes_transferred);
    void post_immediate_completion(io_operation* op);
    void post_deferred_completion(io_operation* op);
    void post_deferred_completions(op_queue<io_operation>& ops);


private:
    void timer_thread();

    HANDLE iocp_;
    HANDLE timer_event_;
    std::thread timer_thread_;
    long shutdown_;
    long stopped_;
    long stop_event_posted_;
    long outstanding_work_;  // 待执行的任务数
    long dispatch_required_;
    std::mutex dispatch_mutex_;
    timer_queue timer_queue_;
    op_queue<io_operation> completed_ops_;
    static accept_ex_fn accept_ex_;
    static accept_ex_socketaddrs_fn accept_ex_socketaddrs_;
    static connect_ex_fn connect_ex_;

private:
    friend class work;
    friend class socket;
};

inline int get_last_error(int default_error = -1)
{
    int last_error = ::WSAGetLastError();
    return last_error != 0 ? last_error : default_error;
}

class io_service::work
{
public:
  explicit work(io_service& io_service) : io_service_(io_service)
  {
	  io_service_.work_started();
  }

  work(const work& other): io_service_(other.io_service_)
  {
	  io_service_.work_started();
  }

  ~work()
  {
	  io_service_.work_finished();
  }

private:
	void operator=(const work& other);

	io_service& io_service_;
};

}
