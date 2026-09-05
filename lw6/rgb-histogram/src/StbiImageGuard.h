#pragma once

#include "stb_image.h"

class StbiImageGuard
{
public:
	explicit StbiImageGuard(unsigned char* data)
		: m_data(data)
	{
	}

	StbiImageGuard(const StbiImageGuard&) = delete;
	StbiImageGuard& operator=(const StbiImageGuard&) = delete;

	StbiImageGuard(StbiImageGuard&& other) noexcept
		: m_data(other.m_data)
	{
		other.m_data = nullptr;
	}

	StbiImageGuard& operator=(StbiImageGuard&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_data = other.m_data;
			other.m_data = nullptr;
		}
		return *this;
	}

	~StbiImageGuard()
	{
		Reset();
	}

	[[nodiscard]] unsigned char* Get() const
	{
		return m_data;
	}

private:
	void Reset()
	{
		if (m_data != nullptr)
		{
			stbi_image_free(m_data);
			m_data = nullptr;
		}
	}

	unsigned char* m_data = nullptr;
};