#pragma once

#include "TaskNode.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class LockBasedThreadPool
{
public:
	explicit LockBasedThreadPool(size_t numThreads);
	~LockBasedThreadPool() noexcept;

	LockBasedThreadPool(const LockBasedThreadPool&) = delete;
	LockBasedThreadPool& operator=(const LockBasedThreadPool&) = delete;

	LockBasedThreadPool(LockBasedThreadPool&&) = delete;
	LockBasedThreadPool& operator=(LockBasedThreadPool&&) = delete;

	template <class F, class... Args>
	auto Enqueue(F&& function, Args&&... args)
		-> std::future<std::invoke_result_t<F, Args...>>;

	void Wait();
	void Shutdown() noexcept;

private:
	void EnqueueTask(std::unique_ptr<TaskNode> task);
	void WorkerLoop() noexcept;
	void NotifyTaskFinished() noexcept;

	std::mutex m_mutex;
	std::condition_variable m_stateChanged;
	std::condition_variable m_allTasksCompleted;
	std::queue<std::unique_ptr<TaskNode>> m_tasks;
	std::vector<std::jthread> m_workers;
	bool m_stopping = false;
	size_t m_pendingTasks = 0;
};

template <class F, class... Args>
auto LockBasedThreadPool::Enqueue(F&& function, Args&&... args)
	-> std::future<std::invoke_result_t<F, Args...>>
{
	using ReturnType = std::invoke_result_t<F, Args...>;

	auto task = std::packaged_task<ReturnType()>(
		[callable = std::forward<F>(function), ... capturedArgs = std::forward<Args>(args)]() mutable -> ReturnType {
			return std::invoke(std::move(callable), std::move(capturedArgs)...);
		});

	auto future = task.get_future();
	EnqueueTask(MakeTaskNode(std::move(task)));
	return future;
}