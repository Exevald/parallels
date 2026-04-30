#include "LockBasedThreadPool.h"
#include "LockFreeThreadPool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

template <typename Pool>
void VerifyTasksExecuteExactlyOnce()
{
	Pool pool(4);
	std::atomic<int> counter{ 0 };
	std::vector<std::future<void>> futures;
	futures.reserve(128);

	for (int index = 0; index < 128; ++index)
	{
		futures.push_back(pool.Enqueue([&counter] {
			counter.fetch_add(1, std::memory_order_relaxed);
		}));
	}

	for (auto& future : futures)
	{
		future.get();
	}
	pool.Wait();

	EXPECT_EQ(counter.load(std::memory_order_relaxed), 128);
}

template <typename Pool>
void VerifyFutureReturnsValue()
{
	Pool pool(2);

	auto future = pool.Enqueue([](int lhs, int rhs) {
		return lhs + rhs;
	}, 20, 22);

	EXPECT_EQ(future.get(), 42);
	pool.Wait();
}

template <typename Pool>
void VerifyFuturePropagatesException()
{
	Pool pool(2);

	auto future = pool.Enqueue([]() -> int {
		throw std::runtime_error("boom");
	});

	EXPECT_THROW(static_cast<void>(future.get()), std::runtime_error);
	pool.Wait();
}

template <typename Pool>
void VerifyWaitBlocksUntilAcceptedTasksFinish()
{
	Pool pool(3);
	std::atomic<int> completed{ 0 };

	for (int index = 0; index < 24; ++index)
	{
		pool.Enqueue([&completed] {
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			completed.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.Wait();
	EXPECT_EQ(completed.load(std::memory_order_relaxed), 24);
}

template <typename Pool>
void VerifyBurstFromManyProducers()
{
	Pool pool(4);
	std::atomic<int> counter{ 0 };
	std::vector<std::thread> producers;
	producers.reserve(6);

	for (int producer = 0; producer < 6; ++producer)
	{
		producers.emplace_back([&pool, &counter] {
			for (int index = 0; index < 50; ++index)
			{
				pool.Enqueue([&counter] {
					counter.fetch_add(1, std::memory_order_relaxed);
				});
			}
		});
	}

	for (auto& producer : producers)
	{
		producer.join();
	}
	pool.Wait();
	EXPECT_EQ(counter.load(std::memory_order_relaxed), 300);
}

template <typename Pool>
void VerifyShutdownPreventsFurtherEnqueue()
{
	Pool pool(2);

	auto future = pool.Enqueue([] {
			return 7;
		});
	EXPECT_EQ(future.get(), 7);

	pool.Shutdown();

	EXPECT_THROW(static_cast<void>(pool.Enqueue([] {
		return 1;
	})), std::runtime_error);
}

template <typename Pool>
void VerifyShutdownCompletesAcceptedTasks()
{
	auto pool = std::make_unique<Pool>(3);
	std::atomic<int> counter{ 0 };

	for (int index = 0; index < 60; ++index)
	{
		pool->Enqueue([&counter] {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.reset();
	EXPECT_EQ(counter.load(std::memory_order_relaxed), 60);
}

TEST(LockFreeThreadPool, ExecutesTasksExactlyOnce)
{
	VerifyTasksExecuteExactlyOnce<LockFreeThreadPool>();
}

TEST(LockFreeThreadPool, ReturnsFutureValue)
{
	VerifyFutureReturnsValue<LockFreeThreadPool>();
}

TEST(LockFreeThreadPool, PropagatesFutureException)
{
	VerifyFuturePropagatesException<LockFreeThreadPool>();
}

TEST(LockFreeThreadPool, WaitBlocksUntilAllAcceptedTasksFinish)
{
	VerifyWaitBlocksUntilAcceptedTasksFinish<LockFreeThreadPool>();
}

TEST(LockFreeThreadPool, HandlesBurstFromManyProducers)
{
	VerifyBurstFromManyProducers<LockFreeThreadPool>();
}

TEST(LockFreeThreadPool, RejectsEnqueueAfterShutdown)
{
	VerifyShutdownPreventsFurtherEnqueue<LockFreeThreadPool>();
}

TEST(LockFreeThreadPool, DestructorCompletesAcceptedTasks)
{
	VerifyShutdownCompletesAcceptedTasks<LockFreeThreadPool>();
}

TEST(LockBasedThreadPool, ExecutesTasksExactlyOnce)
{
	VerifyTasksExecuteExactlyOnce<LockBasedThreadPool>();
}

TEST(LockBasedThreadPool, ReturnsFutureValue)
{
	VerifyFutureReturnsValue<LockBasedThreadPool>();
}

TEST(LockBasedThreadPool, PropagatesFutureException)
{
	VerifyFuturePropagatesException<LockBasedThreadPool>();
}

TEST(LockBasedThreadPool, WaitBlocksUntilAllAcceptedTasksFinish)
{
	VerifyWaitBlocksUntilAcceptedTasksFinish<LockBasedThreadPool>();
}

TEST(LockBasedThreadPool, HandlesBurstFromManyProducers)
{
	VerifyBurstFromManyProducers<LockBasedThreadPool>();
}

TEST(LockBasedThreadPool, RejectsEnqueueAfterShutdown)
{
	VerifyShutdownPreventsFurtherEnqueue<LockBasedThreadPool>();
}

TEST(LockBasedThreadPool, DestructorCompletesAcceptedTasks)
{
	VerifyShutdownCompletesAcceptedTasks<LockBasedThreadPool>();
}