#pragma once

#include "TaskNode.h"

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class LockFreeThreadPool
{
public:
	static constexpr std::ptrdiff_t MaxReadyTasks = std::numeric_limits<int>::max();

	explicit LockFreeThreadPool(size_t numThreads, size_t queueCapacity = 65536);
	~LockFreeThreadPool() noexcept;

	LockFreeThreadPool(const LockFreeThreadPool&) = delete;
	LockFreeThreadPool& operator=(const LockFreeThreadPool&) = delete;

	LockFreeThreadPool(LockFreeThreadPool&&) = delete;
	LockFreeThreadPool& operator=(LockFreeThreadPool&&) = delete;

	template <class F, class... Args>
	auto Enqueue(F&& function, Args&&... args)
		-> std::future<std::invoke_result_t<F, Args...>>;

	void Wait();
	void Shutdown() noexcept;

private:
	void EnqueueTask(std::unique_ptr<TaskNode> task);
	bool TryPopTask(std::unique_ptr<TaskNode>& task) noexcept;
	void WorkerLoop() noexcept;
	void NotifyTaskFinished() noexcept;
	void DrainQueue() noexcept;

	boost::lockfree::queue<TaskNode*> m_tasks;
	std::counting_semaphore<> m_readyTasks{ 0 };
	std::vector<std::jthread> m_workers;
	std::mutex m_waitMutex;
	std::condition_variable m_allTasksCompleted;
	std::atomic<bool> m_stopping{ false };
	std::atomic<size_t> m_pendingTasks{ 0 };
};

template <class F, class... Args>
auto LockFreeThreadPool::Enqueue(F&& function, Args&&... args)
	-> std::future<std::invoke_result_t<F, Args...>>
{
	using ReturnType = std::invoke_result_t<F, Args...>;

	if (m_stopping.load(std::memory_order_acquire))
	{
		throw std::runtime_error("enqueue on stopped LockFreeThreadPool");
	}

	auto task = std::packaged_task<ReturnType()>(
		[callable = std::forward<F>(function), ... capturedArgs = std::forward<Args>(args)]() mutable -> ReturnType {
			return std::invoke(std::move(callable), std::move(capturedArgs)...);
		});

	auto future = task.get_future();
	EnqueueTask(MakeTaskNode(std::move(task)));
	return future;
}