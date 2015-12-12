#pragma once

#include <thread>
#include <mutex>
#include "timer_queue.h"
#include "socket.h"

/*
    在Window是环境下使用IOCP，基本上需要这样几个步骤：
    1.使用Win函数CreateIoCompletionPort()创建一个完成端口对象；
    2.创建一个IO对象，如用于listen的socket对象；
    3.再次调用CreateIoCompletionPort()函数，分别在参数中指定第二步创建的IO对象和第一步创建的完成端口对象。
        由于指定了这两个参数，这一次的调用，只是告诉系统，后续该IO对象上所有的完成事件，都投递到绑定的完成端口上。
    4.创建一个线程或者线程池，用以服务完成端口事件；
    5.所有这些线程调用GetQueuedCompletionStatus()函数等待一个完成端口事件的到来；
    6.进行异步调用，例如WSASend()等操作。在系统执行完异步操作并把事件投递到端口上，
        或者客户自己调用了PostQueuedCompletionStatus()函数，使得在完成端口上等待的一个线程苏醒，执行后续的服务操作。
*/
namespace x
{

class deadline_timer;
class io_service
{
public:
    class work;

public:
    io_service();
    ~io_service();

    int32_t register_handle(socket& s);

    /*
        所有这些需要自己投递完成端口数据包的操作，基本上都是这样一个投递流程：
        1.调用post_immediate_completion(op), 调用work_started()给outstanding_work_加 1
        2.调用post_deferred_completion(op), 由于自行管理，主动将op->ready_置为 1，表明op就绪
        3.调用PostQueuedCompletionStatus(op)进行投递
        4.如果投递失败，则把该op放置到等待dispatch的队列completed_ops_ 中，待下一次do_one()执行时，再次投递
    */
    template<typename Handle>
    void post(Handle h)
    {
        typedef io_operation_extra0<Handle> op;
        op* p = new op(h);
        post_immediate_completion(p);
    }

    template<typename Handle>
    void add_async_timer(deadline_timer& timer, int32_t millisec, Handle h)
    {
        typedef io_operation_extra1<Handle> op;
        op* p = new op(h);

        // timer线程关闭了, 直接执行成功操作
        if (::InterlockedExchangeAdd(&shutdown_, 0) != 0)
        {
            post_immediate_completion(p);
            return;
        }

        std::unique_lock<std::mutex> lock(dispatch_mutex_);
        if (timer_event_ == NULL)
        {   
            timer_event_ = ::CreateWaitableTimer(
                NULL,   // pointer to security attributes
                FALSE,  // flag for manual reset state
                NULL);  // pointer to timer object name

            std::thread t(&io_service::timer_thread, this);
            timer_thread_.swap(t);
        }
        // 新添加的最先过期
        bool my_is_earliest = timer_queue_.enqueue_timer(
            get_milli_secs() + millisec, timer.per_timer_data_, p);

        work_started();
        if (my_is_earliest)
        {
            update_timeout();
        }
    }
    uint32_t cancel_timer(deadline_timer& timer);

    void work_started();
    void work_finished();
    void run();

    void stop();
    void reset();

    // 获取utc 时间, (单位:毫秒)
    static int64_t get_milli_secs();

    // 获取AcceptEx函数指针
    static LPAcceptEx get_AcceptEx(socket& s); 
    // 获取GetAcceptExSockaddrs指针
    static LPGetAcceptExSockaddrs get_GetAcceptExSockaddrs(socket& s); 
    // 获取ConnectEx函数指针
    static LPConnectEx get_ConnectEx(socket& s);

private:
    void shutdown_service();
    void update_timeout();

    void on_pending(io_operation* op);  // 表示异步投递已经成功，但是稍后发送才会完成
    void on_completion(io_operation* op, DWORD last_error, DWORD bytes_transferred);
    void post_immediate_completion(io_operation* op);   // 转 defered_completion
    void post_deferred_completion(io_operation* op);    // post 推迟的完成
    void post_deferred_completions(op_queue<io_operation>& ops);


private:
    void timer_thread();

    HANDLE iocp_;
    HANDLE timer_event_;
    std::thread timer_thread_;
    long shutdown_; // 关闭服务flag
    long stopped_;
    long stop_event_posted_;
    long outstanding_work_;  // 待处理的任务数
    long dispatch_required_; // 记录了由于资源忙，而没有成功投递的操作数
                             // 所有这些操作都记录在队列completed_ops_中
    std::mutex dispatch_mutex_;
    timer_queue timer_queue_;
    op_queue<io_operation> completed_ops_;
    static LPAcceptEx accept_ex_;
    static LPGetAcceptExSockaddrs accept_ex_socketaddrs_;
    static LPConnectEx connect_ex_;

private:
    friend class work;
    friend class socket;
};

inline int32_t get_last_error(int32_t default_error = -1)
{
    int32_t last_error = ::WSAGetLastError();
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
