#include "AudioVisualizer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace radio
{

namespace
{

constexpr int OscilloscopeWidth = 80;
constexpr int OscilloscopeHeight = 21;
constexpr int VisualizationFps = 30;
constexpr size_t MaxStoredSamples = 4096;
constexpr size_t DisplayedSamples = 320;

} // namespace

AudioVisualizer::AudioVisualizer(std::string title, unsigned int channelCount)
	: m_title(std::move(title))
	, m_channelCount(std::max(1U, channelCount))
{
}

void AudioVisualizer::UpdateSamples(const std::vector<int16_t>& samples)
{
	if (samples.empty())
	{
		return;
	}

	std::vector<int16_t> monoSamples;
	monoSamples.reserve(samples.size() / m_channelCount + 1);
	for (size_t index = 0; index < samples.size(); index += m_channelCount)
	{
		monoSamples.push_back(samples[index]);
	}

	std::lock_guard lock(m_samplesMutex);
	if (monoSamples.size() >= MaxStoredSamples)
	{
		m_recentSamples.assign(monoSamples.end() - MaxStoredSamples, monoSamples.end());
		return;
	}

	const size_t overflow = m_recentSamples.size() + monoSamples.size() > MaxStoredSamples
		? m_recentSamples.size() + monoSamples.size() - MaxStoredSamples
		: 0;
	if (overflow > 0)
	{
		m_recentSamples.erase(m_recentSamples.begin(), m_recentSamples.begin() + static_cast<std::ptrdiff_t>(overflow));
	}
	m_recentSamples.insert(m_recentSamples.end(), monoSamples.begin(), monoSamples.end());
}

void AudioVisualizer::Run(const std::atomic_bool& stopRequested)
{
	const auto frameDelay = std::chrono::milliseconds(1000 / VisualizationFps);
	while (!stopRequested.load())
	{
		std::array<std::string, OscilloscopeHeight> rows;
		for (std::string& row : rows)
		{
			row.assign(OscilloscopeWidth, ' ');
		}

		constexpr int midY = OscilloscopeHeight / 2;
		for (int x = 0; x < OscilloscopeWidth; ++x)
		{
			rows[midY][x] = '-';
		}

		const std::vector<int16_t> recentSamples = SnapshotSamples();
		if (!recentSamples.empty())
		{
			const size_t windowSize = std::min(DisplayedSamples, recentSamples.size());
			const size_t startIndex = recentSamples.size() - windowSize;
			for (int x = 0; x < OscilloscopeWidth; ++x)
			{
				const size_t sampleOffset = static_cast<size_t>(x) * std::max<size_t>(1, windowSize / OscilloscopeWidth);
				const size_t sampleIndex = std::min(recentSamples.size() - 1, startIndex + sampleOffset);
				const float normalized = static_cast<float>(recentSamples[sampleIndex])
					/ static_cast<float>(std::numeric_limits<int16_t>::max());
				const int y = std::clamp(
					static_cast<int>(std::lround(static_cast<float>(midY) - normalized * static_cast<float>(midY - 1))),
					0,
					OscilloscopeHeight - 1);
				rows[y][x] = '*';
			}
		}

		constexpr int cursorX = OscilloscopeWidth / 2;
		for (int y = 0; y < OscilloscopeHeight; ++y)
		{
			rows[y][cursorX] = (y == midY) ? '+' : '|';
		}

		if (!m_screenInitialized)
		{
			std::cout << "\x1b[2J";
			m_screenInitialized = true;
		}
		std::cout << m_title << "\n";
		std::cout << "Latest samples: " << recentSamples.size() << "\n";
		for (const std::string& row : rows)
		{
			std::cout << row << '\n';
		}
		std::cout.flush();

		std::this_thread::sleep_for(frameDelay);
	}
}

std::vector<int16_t> AudioVisualizer::SnapshotSamples() const
{
	std::lock_guard lock(m_samplesMutex);
	return m_recentSamples;
}
} // namespace radio
