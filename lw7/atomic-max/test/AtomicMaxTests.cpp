#include "AtomicMax.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

TEST(AtomicMax, ReturnsInitialValue)
{
	const AtomicMax value(17);

	EXPECT_EQ(value.GetValue(), 17);
}

TEST(AtomicMax, UpdatesToGreaterValue)
{
	AtomicMax value(10);

	value.Update(42);

	EXPECT_EQ(value.GetValue(), 42);
}

TEST(AtomicMax, IgnoresSmallerAndEqualValues)
{
	AtomicMax value(100);

	value.Update(99);
	value.Update(100);

	EXPECT_EQ(value.GetValue(), 100);
}

TEST(AtomicMax, MatchesLockBasedVersionOnSameInput)
{
	AtomicMax<std::int64_t> atomicMax(0);
	AtomicMaxWithLock<std::int64_t> lockedMax(0);

	for (const std::vector<std::int64_t> values{ 4, 11, 2, 11, 8, 150, 149, 151, 33 };
		const auto value : values)
	{
		atomicMax.Update(value);
		lockedMax.Update(value);
	}

	EXPECT_EQ(atomicMax.GetValue(), lockedMax.GetValue());
}

TEST(AtomicMax, ComputesGlobalMaximumAcrossThreads)
{
	AtomicMax atomicMax(-1);
	std::vector<std::thread> workers;
	workers.reserve(8);

	for (int threadIndex = 0; threadIndex < 8; ++threadIndex)
	{
		workers.emplace_back([threadIndex, &atomicMax] {
			for (int iteration = 0; iteration < 20000; ++iteration)
			{
				atomicMax.Update(threadIndex * 20000 + iteration);
			}
		});
	}

	for (auto& worker : workers)
	{
		worker.join();
	}

	EXPECT_EQ(atomicMax.GetValue(), 8 * 20000 - 1);
}

TEST(AtomicMax, SurvivesContentionAndKeepsMaximum)
{
	AtomicMax atomicMax(0);
	std::atomic referenceMax{ 0 };
	std::vector<std::thread> workers;
	workers.reserve(12);

	for (int threadIndex = 0; threadIndex < 12; ++threadIndex)
	{
		workers.emplace_back([threadIndex, &atomicMax, &referenceMax] {
			for (int iteration = 0; iteration < 5000; ++iteration)
			{
				const int value = (iteration * 37 + threadIndex * 101) % 100000;
				atomicMax.Update(value);

				int current = referenceMax.load(std::memory_order_relaxed);
				while (value > current
					&& !referenceMax.compare_exchange_weak(
						current,
						value,
						std::memory_order_relaxed,
						std::memory_order_relaxed))
				{
				}
			}
		});
	}

	for (auto& worker : workers)
	{
		worker.join();
	}

	EXPECT_EQ(atomicMax.GetValue(), referenceMax.load(std::memory_order_relaxed));
}