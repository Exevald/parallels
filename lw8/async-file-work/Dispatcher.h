#pragma once

#include <functional>
#include <mutex>
#include <queue>

class Dispatcher
{
	std::mutex mutex;
	std::queue<std::function<void()>> tasks;
	std::condition_variable workAvailable;
	bool isStopped = false;
	std::vector<std::thread> workers;

public:
	explicit Dispatcher(const size_t threads = 4)
	{
		for (size_t i = 0; i < threads; ++i)
		{
			workers.emplace_back([this] {
				while (true)
				{
					std::function<void()> task;
					{
						std::unique_lock lock(mutex);
						workAvailable.wait(lock, [this] {
							return isStopped || !tasks.empty();
						});
						if (isStopped && tasks.empty())
						{
							return;
						}
						task = std::move(tasks.front());
						tasks.pop();
					}
					task();
				}
			});
		}
	}

	~Dispatcher()
	{
		{
			std::unique_lock lock(mutex);
			isStopped = true;
		}
		workAvailable.notify_all();
		for (auto& t : workers)
		{
			t.join();
		}
	}

	void Post(std::function<void()> task)
	{
		{
			std::unique_lock lock(mutex);
			tasks.push(std::move(task));
		}
		workAvailable.notify_one();
	}
};