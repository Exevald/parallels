#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <utility>

struct MyAwaiter
{
	int a, b;

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<>) const noexcept {}
	int await_resume() const noexcept
	{
		return a + b;
	}
};

class MyTask
{
public:
	struct promise_type
	{
		std::exception_ptr exceptionPtr = nullptr;

		MyTask get_return_object()
		{
			return MyTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
		}

		std::suspend_never initial_suspend() noexcept { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void return_void() noexcept {}

		void unhandled_exception() noexcept
		{
			exceptionPtr = std::current_exception();
		}
	};

	explicit MyTask(const std::coroutine_handle<promise_type> handle)
		: m_handle(handle)
	{
		if (!m_handle)
		{
			throw std::runtime_error("Failed to create coroutine handle");
		}
	}

	~MyTask()
	{
		if (m_handle)
		{
			m_handle.destroy();
		}
	}

	MyTask(const MyTask&) = delete;
	MyTask& operator=(const MyTask&) = delete;

	MyTask(MyTask&& other) noexcept
		: m_handle(std::exchange(other.m_handle, nullptr))
	{
	}

	MyTask& operator=(MyTask&& other) noexcept
	{
		if (this != &other)
		{
			if (m_handle)
			{
				m_handle.destroy();
			}
			m_handle = std::exchange(other.m_handle, nullptr);
		}
		return *this;
	}

	void Resume() const
	{
		if (!m_handle)
		{
			throw std::runtime_error("Attempt to resume a null handle");
		}

		if (!m_handle.done())
		{
			m_handle.resume();
		}

		if (m_handle.promise().exceptionPtr)
		{
			std::rethrow_exception(m_handle.promise().exceptionPtr);
		}
	}

private:
	std::coroutine_handle<promise_type> m_handle = nullptr;
};

MyTask CoroutineWithAwait(int x, int y)
{
	std::cout << "Before await\n";
	const int result = co_await MyAwaiter{ x, y };
	std::cout << result << "\n";
	std::cout << "After await\n";
}

int main()
{
	try
	{
		const auto task = CoroutineWithAwait(30, 12);
		std::cout << "Before resume\n";
		task.Resume();
		std::cout << "After resume\n";

		CoroutineWithAwait(5, 10).Resume();
		std::cout << "End of main\n";
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}