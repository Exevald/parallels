#pragma once

#include <chrono>

namespace timer
{

class Stopwatch
{
public:
	Stopwatch();

	void Start();
	void Stop();
	[[nodiscard]] double ElapsedSeconds() const;

private:
	std::chrono::steady_clock::time_point m_start;
	std::chrono::steady_clock::time_point m_end;
	bool m_running = false;
};

} // namespace timer