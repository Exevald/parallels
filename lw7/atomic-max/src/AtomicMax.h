#pragma once

#include <atomic>
#include <concepts>
#include <mutex>

template <typename T>
concept AtomicMaxValue = std::totally_ordered<T> && std::is_trivially_copyable_v<T>;

template <AtomicMaxValue T>
class AtomicMax
{
public:
	explicit AtomicMax(T value) noexcept
		: m_value(value)
	{
	}

	void Update(T newValue) noexcept
	{
		T current = m_value.load(std::memory_order_relaxed);
		while (newValue > current)
		{
			if (m_value.compare_exchange_weak(
					current,
					newValue,
					std::memory_order_release,
					std::memory_order_relaxed))
			{
				return;
			}
		}
	}

	[[nodiscard]] T GetValue() const noexcept
	{
		return m_value.load(std::memory_order_acquire);
	}

private:
	std::atomic<T> m_value;
};

template <AtomicMaxValue T>
class AtomicMaxWithLock
{
public:
	explicit AtomicMaxWithLock(T value) noexcept
		: m_value(value)
	{
	}

	void Update(T newValue) noexcept
	{
		std::lock_guard lock(m_mutex);
		if (newValue > m_value)
		{
			m_value = newValue;
		}
	}

	[[nodiscard]] T GetValue() const noexcept
	{
		std::lock_guard lock(m_mutex);
		return m_value;
	}

private:
	mutable std::mutex m_mutex;
	T m_value;
};