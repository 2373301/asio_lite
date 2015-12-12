#pragma once

#include <thread>
#include <mutex>
#include "timer_queue.h"
#include "socket.h"

/*
    ��Window�ǻ�����ʹ��IOCP����������Ҫ�����������裺
    1.ʹ��Win����CreateIoCompletionPort()����һ����ɶ˿ڶ���
    2.����һ��IO����������listen��socket����
    3.�ٴε���CreateIoCompletionPort()�������ֱ��ڲ�����ָ���ڶ���������IO����͵�һ����������ɶ˿ڶ���
        ����ָ������������������һ�εĵ��ã�ֻ�Ǹ���ϵͳ��������IO���������е�����¼�����Ͷ�ݵ��󶨵���ɶ˿��ϡ�
    4.����һ���̻߳����̳߳أ����Է�����ɶ˿��¼���
    5.������Щ�̵߳���GetQueuedCompletionStatus()�����ȴ�һ����ɶ˿��¼��ĵ�����
    6.�����첽���ã�����WSASend()�Ȳ�������ϵͳִ�����첽���������¼�Ͷ�ݵ��˿��ϣ�
        ���߿ͻ��Լ�������PostQueuedCompletionStatus()������ʹ������ɶ˿��ϵȴ���һ���߳����ѣ�ִ�к����ķ��������
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
        ������Щ��Ҫ�Լ�Ͷ����ɶ˿����ݰ��Ĳ����������϶�������һ��Ͷ�����̣�
        1.����post_immediate_completion(op), ����work_started()��outstanding_work_�� 1
        2.����post_deferred_completion(op), �������й���������op->ready_��Ϊ 1������op����
        3.����PostQueuedCompletionStatus(op)����Ͷ��
        4.���Ͷ��ʧ�ܣ���Ѹ�op���õ��ȴ�dispatch�Ķ���completed_ops_ �У�����һ��do_one()ִ��ʱ���ٴ�Ͷ��
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

        // timer�̹߳ر���, ֱ��ִ�гɹ�����
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
        // ����ӵ����ȹ���
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

    // ��ȡutc ʱ��, (��λ:����)
    static int64_t get_milli_secs();

    // ��ȡAcceptEx����ָ��
    static LPAcceptEx get_AcceptEx(socket& s); 
    // ��ȡGetAcceptExSockaddrsָ��
    static LPGetAcceptExSockaddrs get_GetAcceptExSockaddrs(socket& s); 
    // ��ȡConnectEx����ָ��
    static LPConnectEx get_ConnectEx(socket& s);

private:
    void shutdown_service();
    void update_timeout();

    void on_pending(io_operation* op);  // ��ʾ�첽Ͷ���Ѿ��ɹ��������Ժ��ͲŻ����
    void on_completion(io_operation* op, DWORD last_error, DWORD bytes_transferred);
    void post_immediate_completion(io_operation* op);   // ת defered_completion
    void post_deferred_completion(io_operation* op);    // post �Ƴٵ����
    void post_deferred_completions(op_queue<io_operation>& ops);


private:
    void timer_thread();

    HANDLE iocp_;
    HANDLE timer_event_;
    std::thread timer_thread_;
    long shutdown_; // �رշ���flag
    long stopped_;
    long stop_event_posted_;
    long outstanding_work_;  // �������������
    long dispatch_required_; // ��¼��������Դæ����û�гɹ�Ͷ�ݵĲ�����
                             // ������Щ��������¼�ڶ���completed_ops_��
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
