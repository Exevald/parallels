#pragma once

#include <coroutine>
#include <queue>

class Task
{
public:
	struct promise_type
	{
		std::exception_ptr exceptionPtr;
		std::coroutine_handle<> waiter;

		Task get_return_object() { return Task{
			std::coroutine_handle<promise_type>::from_promise(*this)
		}; }
		std::suspend_never initial_suspend() { return {}; }

		struct FinalAwaiter
		{
			bool await_ready() noexcept { return false; }
			std::coroutine_handle<> await_suspend(
				const std::coroutine_handle<promise_type> waiterHandle) noexcept
			{
				if (waiterHandle.promise().waiter)
				{
					return waiterHandle.promise().waiter;
				}
				return std::noop_coroutine();
			}
			void await_resume() noexcept {}
		};

		FinalAwaiter final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() { exceptionPtr = std::current_exception(); }
	};

	std::coroutine_handle<promise_type> handle;

	explicit Task(std::coroutine_handle<promise_type> h)
		: handle(h)
	{
	}

	~Task()
	{
		if (handle)
		{
			handle.destroy();
		}
	}

	Task(Task&& other) noexcept
		: handle(std::exchange(other.handle, nullptr))
	{
	}

	Task(const Task&) = delete;

	bool await_ready() { return handle.done(); }

	void await_suspend(std::coroutine_handle<> awaiting_handle)
	{
		handle.promise().waiter = awaiting_handle;
	}

	void await_resume()
	{
		if (handle.promise().exceptionPtr)
			std::rethrow_exception(handle.promise().exceptionPtr);
	}
};