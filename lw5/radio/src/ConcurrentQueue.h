#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>

namespace radio
{

template <typename T>
class BoundedQueue
{
public:
    explicit BoundedQueue(size_t capacity)
        : m_capacity(capacity)
    {
    }

    void PushDropOldest(T value)
    {
        {
            std::lock_guard lock(m_mutex);
            if (m_closed)
            {
                return;
            }
            if (m_queue.size() >= m_capacity)
            {
                m_queue.pop_front();
            }
            m_queue.push_back(std::move(value));
        }
        m_cv.notify_one();
    }

    [[nodiscard]] std::optional<T> WaitPop()
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [&]() {
            return m_closed || !m_queue.empty();
        });

        if (m_queue.empty())
        {
            return std::nullopt;
        }

        T value = std::move(m_queue.front());
        m_queue.pop_front();
        return value;
    }

    [[nodiscard]] std::optional<T> TryPopNewest()
    {
        std::lock_guard lock(m_mutex);
        if (m_queue.empty())
        {
            return std::nullopt;
        }

        T value = std::move(m_queue.back());
        m_queue.clear();
        return value;
    }

    [[nodiscard]] size_t Size() const
    {
        std::lock_guard lock(m_mutex);
        return m_queue.size();
    }

    void Close()
    {
        {
            std::lock_guard lock(m_mutex);
            m_closed = true;
        }
        m_cv.notify_all();
    }

private:
    const size_t m_capacity;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<T> m_queue;
    bool m_closed = false;
};

}
