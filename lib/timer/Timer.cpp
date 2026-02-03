#include "Timer.h"

namespace timer
{

Stopwatch::Stopwatch()
{
	Start();
}

void Stopwatch::Start()
{
	m_start = std::chrono::steady_clock::now();
	m_running = true;
}

void Stopwatch::Stop()
{
	m_end = std::chrono::steady_clock::now();
	m_running = false;
}

double Stopwatch::ElapsedSeconds() const
{
	auto end = m_running ? std::chrono::steady_clock::now() : m_end;
	return std::chrono::duration_cast<std::chrono::duration<double>>(end - m_start).count();
}

} // namespace timer