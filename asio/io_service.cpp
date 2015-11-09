
#if defined(_MSC_VER)

#define _CRT_SECURE_NO_WARNINGS
#include <time.h>
#include <sys/timeb.h>
#include <limits>
#include "io_service.h"
#include "deadline_timer.h"

namespace asio
{

accept_ex_fn io_service::accept_ex_;
accept_ex_socketaddrs_fn io_service::accept_ex_socketaddrs_;
connect_ex_fn io_service::connect_ex_;

struct work_finished_on_block_exit
{
	~work_finished_on_block_exit()
	{
		io_service_->work_finished();
	}
	io_service* io_service_;
};

io_service::io_service()
	: timer_event_(0)
	, shutdown_(0)
	, stopped_(0)
	, stop_event_posted_(0)
	, outstanding_work_(0)
	, dispatch_required_(0)
{   
    // 初始化 winsock
	WSADATA wsaData;
	::WSAStartup(MAKEWORD(1, 1), &wsaData);

    // 创建iocp 完成端口
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);	
}

io_service::~io_service()
{
	shutdown_service();
}

void io_service::work_started()
{
	::InterlockedIncrement(&outstanding_work_);
}

// Notify that some work has finished.
void io_service::work_finished()
{
	if (::InterlockedDecrement(&outstanding_work_) == 0)
	{
		stop();
	}
}

int io_service::register_handle(asio::socket& s)
{
	if (::CreateIoCompletionPort((HANDLE)s.socket_, iocp_, 0, 0) == 0)
	{
		return get_last_error(-1);
	}
	return 0;
}

void io_service::stop()
{
	if (::InterlockedExchange(&stopped_, 1) == 0)
	{
		if (::InterlockedExchange(&stop_event_posted_, 1) == 0)
		{
			::PostQueuedCompletionStatus(iocp_, 0, 0, 0);
		}
	}
}

void io_service::reset()
{
	::InterlockedExchange(&stopped_, 0);
}


void io_service::update_timeout()
{
	const long max_timeout_msec = (std::numeric_limits<long>::max)();
	long timeout_msec = timer_queue_.wait_duration_msec(max_timeout_msec);
	if (timeout_msec < max_timeout_msec)
	{
		LARGE_INTEGER timeout;
		timeout.QuadPart = -10000 * timeout_msec; 
		::SetWaitableTimer(timer_event_, &timeout, max_timeout_msec, 0, 0, FALSE);
	}
}

unsigned int io_service::cancel_timer(deadline_timer& timer)
{
	if (::InterlockedExchangeAdd(&shutdown_, 0) != 0)
		return 0;
	std::unique_lock<std::mutex> lock(dispatch_mutex_);
	op_queue<io_operation> ops;
	unsigned int n = timer_queue_.cancel_timer(timer.per_timer_data_, ops);
	post_deferred_completions(ops);
	return n;
}

void io_service::on_pending(io_operation* op)
{
	if (::InterlockedCompareExchange(&op->ready_, 1, 0) == 1)
	{
		if (!::PostQueuedCompletionStatus(iocp_, 0, io_key_overlapped_contains_result, op))
		{
			std::unique_lock<std::mutex> lock(dispatch_mutex_);
			completed_ops_.push(op);
			::InterlockedExchange(&dispatch_required_, 1);
		}
	}
}

void io_service::on_completion(io_operation* op, DWORD last_error, DWORD bytes_transferred)
{
	op->ready_ = 1;

	op->Offset = last_error;
	op->OffsetHigh = bytes_transferred;

	if (!::PostQueuedCompletionStatus(iocp_, 0, io_key_overlapped_contains_result, op))
	{
		std::unique_lock<std::mutex> lock(dispatch_mutex_);
		completed_ops_.push(op);
		::InterlockedExchange(&dispatch_required_, 1);
	}
}

void io_service::post_immediate_completion(io_operation* op)
{
	work_started();
	post_deferred_completion(op);
}

void io_service::post_deferred_completion(io_operation* op)
{
	op->ready_ = 1;

	if (!::PostQueuedCompletionStatus(iocp_, 0, io_key_null, op))
	{
		std::unique_lock<std::mutex> lock(dispatch_mutex_);
		completed_ops_.push(op);
		::InterlockedExchange(&dispatch_required_, 1);
	}
}

void io_service::post_deferred_completions(op_queue<io_operation>& ops)
{
	while (io_operation* op = ops.front())
	{
		ops.pop();
		post_deferred_completion(op);
	}
}

long long io_service::get_time()
{
	_timeb timebuffer;
	_ftime(&timebuffer);
	long long now = timebuffer.time * 1000 + timebuffer.millitm;
	return now;
}

void io_service::timer_thread(void)
{
	while (::InterlockedExchangeAdd(&shutdown_, 0) == 0)
	{
		WaitForSingleObject(timer_event_, INFINITE);
		::InterlockedExchange(&dispatch_required_, 1);
		::PostQueuedCompletionStatus(iocp_, 0, io_key_dispatch, 0);
	}
}

void io_service::run()
{
	if (::InterlockedExchangeAdd(&outstanding_work_, 0) == 0)
	{
		stop();
		return;
	}

	while (true)
	{
		if (::InterlockedCompareExchange(&dispatch_required_, 0, 1) == 1)
		{
			std::unique_lock<std::mutex> lock(dispatch_mutex_);
			op_queue<io_operation> ops;
			ops.push(completed_ops_);
			timer_queue_.get_ready_timers(ops);
			post_deferred_completions(ops);
			update_timeout();
		}
		
		DWORD bytes_transferred = 0;
		ULONG_PTR completion_key = 0;
		LPOVERLAPPED overlapped = 0;
		::SetLastError(0);

		BOOL ok = ::GetQueuedCompletionStatus(iocp_, &bytes_transferred,
			&completion_key, &overlapped, INFINITE);
		DWORD last_error = ::GetLastError();

		if (overlapped)
		{
			io_operation* op = static_cast<io_operation*>(overlapped);
			if (completion_key == io_key_overlapped_contains_result)
			{
				last_error = op->Offset;
				bytes_transferred = op->OffsetHigh;
			}
			else
			{
				op->Offset = last_error;
				op->OffsetHigh = bytes_transferred;
			}

			if (::InterlockedCompareExchange(&op->ready_, 1, 0) == 1)
			{
				work_finished_on_block_exit on_exit = { this };
				(void)on_exit;

				op->complete(*this, last_error, bytes_transferred);
				continue;
			}
		}
		else if (!ok)
		{
			if (last_error == WAIT_TIMEOUT)
			{
				continue;
			}

			return;
		}
		else if (completion_key == io_key_dispatch)
		{
			continue;
		}
		else
		{
			::InterlockedExchange(&stop_event_posted_, 0);
			if (::InterlockedExchangeAdd(&stopped_, 0) != 0)
			{
				if (::InterlockedExchange(&stop_event_posted_, 1) == 0)
				{
					::PostQueuedCompletionStatus(iocp_, 0, 0, 0);
				}
				return;
			}
		}
	}
}

void io_service::shutdown_service()
{
	::InterlockedExchange(&shutdown_, 1);

	if (timer_event_)
	{
		LARGE_INTEGER timeout;
		timeout.QuadPart = 1;
		::SetWaitableTimer(timer_event_, &timeout, 1, 0, 0, FALSE);
		timer_thread_.join();
		::CloseHandle(timer_event_);
		timer_event_ = 0;
	}

	while (::InterlockedExchangeAdd(&outstanding_work_, 0) > 0)
	{
		op_queue<io_operation> ops;
		timer_queue_.get_all_timers(ops);
		ops.push(completed_ops_);
		if (!ops.empty())
		{
			while (io_operation* op = ops.front())
			{
				ops.pop();
				::InterlockedDecrement(&outstanding_work_);
				op->destroy();
			}
		}
		else
		{
			DWORD bytes_transferred = 0;
			ULONG_PTR completion_key = 0;
			LPOVERLAPPED overlapped = 0;
			::GetQueuedCompletionStatus(iocp_, &bytes_transferred,
				&completion_key, &overlapped, INFINITE);
			if (overlapped)
			{
				::InterlockedDecrement(&outstanding_work_);
				static_cast<io_operation*>(overlapped)->destroy();
			}
		}
	}

	::CloseHandle(iocp_);
}

accept_ex_fn io_service::get_accept_ex(socket& s)
{
	void* ptr = InterlockedCompareExchangePointer((void**)&accept_ex_, 0, 0);
	if (!ptr)
	{
		GUID guid = {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}};
		DWORD bytes = 0;
		if (::WSAIoctl(s.socket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0)
		{
			ptr = (void*)0x100;
		}

		InterlockedExchangePointer((PVOID*)&accept_ex_, ptr);
	}
	return reinterpret_cast<accept_ex_fn>(ptr == (void*)0x100 ? 0 : ptr);
}

accept_ex_socketaddrs_fn io_service::get_accept_ex_socketaddrs(socket& s)
{
	void* ptr = InterlockedCompareExchangePointer((void**)&accept_ex_socketaddrs_, 0, 0);
	if (!ptr)
	{
		GUID guid = {0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}};
		DWORD bytes = 0;
		if (::WSAIoctl(s.socket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0)
		{
			ptr = (void*)0x100;
		}

		InterlockedExchangePointer((PVOID*)&connect_ex_, ptr);
	}
	return reinterpret_cast<accept_ex_socketaddrs_fn>(ptr == (void*)0x100 ? 0 : ptr);
}

connect_ex_fn io_service::get_connect_ex(socket& s)
{
	void* ptr = InterlockedCompareExchangePointer((void**)&connect_ex_, 0, 0);
	if (!ptr)
	{
		GUID guid = {0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}};

		DWORD bytes = 0;
		if (::WSAIoctl(s.socket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0)
		{
			ptr = (void*)0x100;
		}

		InterlockedExchangePointer((PVOID*)&connect_ex_, ptr);
	}
	return reinterpret_cast<connect_ex_fn>(ptr == (void*)0x100 ? 0 : ptr);
}

}

#endif
