#pragma once

#include <future>
#include <memory>
#include <utility>

class TaskNode
{
public:
	virtual ~TaskNode() = default;
	virtual void Execute() noexcept = 0;
};

template <typename ReturnType>
class PackagedTaskNode final : public TaskNode
{
public:
	explicit PackagedTaskNode(std::packaged_task<ReturnType()>&& task)
		: m_task(std::move(task))
	{
	}

	void Execute() noexcept override
	{
		try
		{
			m_task();
		}
		catch (...)
		{
		}
	}

private:
	std::packaged_task<ReturnType()> m_task;
};

template <typename ReturnType>
std::unique_ptr<TaskNode> MakeTaskNode(std::packaged_task<ReturnType()>&& task)
{
	return std::make_unique<PackagedTaskNode<ReturnType>>(std::move(task));
}