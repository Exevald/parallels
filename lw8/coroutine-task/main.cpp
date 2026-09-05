#include <coroutine>
#include <exception>
#include <iostream>
#include <string>
#include <utility>

class MyTask
{
public:
	struct promise_type
	{
		std::string result;
		std::exception_ptr exceptionPtr = nullptr;

		MyTask get_return_object()
		{
			return MyTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
		}

		std::suspend_never initial_suspend() noexcept { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void return_value(std::string value)
		{
			result = std::move(value);
		}

		void unhandled_exception()
		{
			exceptionPtr = std::current_exception();
		}
	};

	std::string GetResult() const
	{
		if (!m_handle)
		{
			throw std::runtime_error("Empty coroutine handle");
		}
		if (m_handle.promise().exceptionPtr)
		{
			std::rethrow_exception(m_handle.promise().exceptionPtr);
		}

		return m_handle.promise().result;
	}

	explicit MyTask(const std::coroutine_handle<promise_type> handle)
		: m_handle(handle)
	{
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

	~MyTask()
	{
		if (m_handle)
		{
			m_handle.destroy();
		}
	}

private:
	std::coroutine_handle<promise_type> m_handle = nullptr;
};

MyTask SimpleCoroutine()
{
	co_return "Hello from coroutine!";
}

int main()
{
	try
	{
		const MyTask task = SimpleCoroutine();
		std::cout << task.GetResult() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}