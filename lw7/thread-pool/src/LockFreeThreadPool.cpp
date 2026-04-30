#include "LockFreeThreadPool.h"

#include <chrono>

LockFreeThreadPool::LockFreeThreadPool(const size_t numThreads, const size_t queueCapacity)
	: m_tasks(queueCapacity)
{
	if (numThreads == 0)
	{
		throw std::invalid_argument("LockFreeThreadPool requires at least one worker");
	}
	if (queueCapacity == 0)
	{
		throw std::invalid_argument("LockFreeThreadPool requires positive queue capacity");
	}
	if (queueCapacity > static_cast<size_t>(MaxReadyTasks))
	{
		throw std::invalid_argument("LockFreeThreadPool queue capacity exceeds semaphore limit");
	}

	m_workers.reserve(numThreads);
	for (size_t index = 0; index < numThreads; ++index)
	{
		m_workers.emplace_back(&LockFreeThreadPool::WorkerLoop, this);
	}
}

LockFreeThreadPool::~LockFreeThreadPool() noexcept
{
	Shutdown();
}

void LockFreeThreadPool::Wait()
{
	std::unique_lock lock(m_waitMutex);
	m_allTasksCompleted.wait(lock, [this] {
		return m_pendingTasks.load(std::memory_order_acquire) == 0;
	});
}

void LockFreeThreadPool::Shutdown() noexcept
{
	if (bool expected = false; !m_stopping.compare_exchange_strong(
			expected,
			true,
			std::memory_order_acq_rel,
			std::memory_order_acquire))
	{
		return;
	}

	m_readyTasks.release(static_cast<std::ptrdiff_t>(m_workers.size()));
	m_workers.clear();
	DrainQueue();
}

void LockFreeThreadPool::EnqueueTask(std::unique_ptr<TaskNode> task)
{
	if (m_stopping.load(std::memory_order_acquire))
	{
		throw std::runtime_error("enqueue on stopped LockFreeThreadPool");
	}

	TaskNode* rawTask = task.get();
	while (!m_tasks.push(rawTask))
	{
		if (m_stopping.load(std::memory_order_acquire))
		{
			throw std::runtime_error("enqueue on stopped LockFreeThreadPool");
		}
		std::this_thread::yield();
	}

	task.release();
	m_pendingTasks.fetch_add(1, std::memory_order_release);
	m_readyTasks.release();
}

bool LockFreeThreadPool::TryPopTask(std::unique_ptr<TaskNode>& task) noexcept
{
	TaskNode* rawTask = nullptr;
	if (!m_tasks.pop(rawTask))
	{
		return false;
	}

	task.reset(rawTask);
	return true;
}

void LockFreeThreadPool::WorkerLoop() noexcept
{
	while (true)
	{
		m_readyTasks.acquire();

		std::unique_ptr<TaskNode> task;
		if (TryPopTask(task))
		{
			task->Execute();
			NotifyTaskFinished();
			continue;
		}

		if (m_stopping.load(std::memory_order_acquire)
			&& m_pendingTasks.load(std::memory_order_acquire) == 0)
		{
			return;
		}

		if (m_stopping.load(std::memory_order_acquire))
		{
			while (m_pendingTasks.load(std::memory_order_acquire) != 0)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(50));
			}
			return;
		}

		std::this_thread::yield();
	}
}

void LockFreeThreadPool::NotifyTaskFinished() noexcept
{
	if (m_pendingTasks.fetch_sub(1, std::memory_order_acq_rel) == 1)
	{
		m_allTasksCompleted.notify_all();
	}
}

void LockFreeThreadPool::DrainQueue() noexcept
{
	std::unique_ptr<TaskNode> task;
	while (TryPopTask(task))
	{
		task.reset();
	}
}