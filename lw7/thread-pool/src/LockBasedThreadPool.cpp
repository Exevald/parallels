#include "LockBasedThreadPool.h"

#include <utility>

LockBasedThreadPool::LockBasedThreadPool(size_t numThreads)
{
	if (numThreads == 0)
	{
		throw std::invalid_argument("LockBasedThreadPool requires at least one worker");
	}

	m_workers.reserve(numThreads);
	for (size_t index = 0; index < numThreads; ++index)
	{
		m_workers.emplace_back(&LockBasedThreadPool::WorkerLoop, this);
	}
}

LockBasedThreadPool::~LockBasedThreadPool() noexcept
{
	Shutdown();
}

void LockBasedThreadPool::Wait()
{
	std::unique_lock lock(m_mutex);
	m_allTasksCompleted.wait(lock, [this] {
		return m_pendingTasks == 0;
	});
}

void LockBasedThreadPool::Shutdown() noexcept
{
	{
		std::lock_guard lock(m_mutex);
		if (m_stopping)
		{
			return;
		}
		m_stopping = true;
	}

	m_stateChanged.notify_all();
	m_workers.clear();
}

void LockBasedThreadPool::EnqueueTask(std::unique_ptr<TaskNode> task)
{
	{
		std::lock_guard lock(m_mutex);
		if (m_stopping)
		{
			throw std::runtime_error("enqueue on stopped LockBasedThreadPool");
		}

		m_tasks.push(std::move(task));
		++m_pendingTasks;
	}

	m_stateChanged.notify_one();
}

void LockBasedThreadPool::WorkerLoop() noexcept
{
	while (true)
	{
		std::unique_ptr<TaskNode> task;
		{
			std::unique_lock lock(m_mutex);
			m_stateChanged.wait(lock, [this] {
				return m_stopping || !m_tasks.empty();
			});

			if (m_tasks.empty())
			{
				if (m_stopping)
				{
					return;
				}
				continue;
			}

			task = std::move(m_tasks.front());
			m_tasks.pop();
		}

		task->Execute();
		NotifyTaskFinished();
	}
}

void LockBasedThreadPool::NotifyTaskFinished() noexcept
{
	std::lock_guard lock(m_mutex);
	--m_pendingTasks;
	if (m_pendingTasks == 0)
	{
		m_allTasksCompleted.notify_all();
	}
}