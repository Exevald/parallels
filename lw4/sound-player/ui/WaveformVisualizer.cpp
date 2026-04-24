#include "ui/WaveformVisualizer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace
{
constexpr int OSCILLOSCOPE_WIDTH = 80;
constexpr int OSCILLOSCOPE_HEIGHT = 21;
constexpr int VISUALIZATION_FPS = 60;
constexpr float DISPLAY_WINDOW_SECONDS = 0.04f;
} // namespace

void waveformVisualizer::Show(const RenderedComposition& composition)
{
	if (composition.samples.empty())
	{
		return;
	}

	const auto start = std::chrono::steady_clock::now();
	const auto frameDelay = std::chrono::milliseconds(1000 / VISUALIZATION_FPS);
	while (true)
	{
		const auto now = std::chrono::steady_clock::now();
		const float currentSeconds = std::min(
			std::chrono::duration<float>(now - start).count(),
			composition.durationSeconds);

		std::array<std::string, OSCILLOSCOPE_HEIGHT> rows;
		for (std::string& row : rows)
		{
			row.assign(OSCILLOSCOPE_WIDTH, ' ');
		}

		constexpr int midY = OSCILLOSCOPE_HEIGHT / 2;
		for (int x = 0; x < OSCILLOSCOPE_WIDTH; ++x)
		{
			rows[midY][x] = '-';
		}

		const auto centerSample = static_cast<std::size_t>(currentSeconds * static_cast<float>(SAMPLE_RATE));
		constexpr auto halfWindow = static_cast<std::size_t>(DISPLAY_WINDOW_SECONDS * static_cast<float>(SAMPLE_RATE) * 0.5f);
		const std::size_t startSample = centerSample > halfWindow ? centerSample - halfWindow : 0;
		for (int x = 0; x < OSCILLOSCOPE_WIDTH; ++x)
		{
			const std::size_t sampleIndex = std::min(
				composition.samples.size() - 1,
				startSample + static_cast<std::size_t>(x) * std::max<std::size_t>(1, halfWindow * 2 / OSCILLOSCOPE_WIDTH));
			const float normalized = static_cast<float>(composition.samples[sampleIndex])
				/ static_cast<float>(std::numeric_limits<std::int16_t>::max());
			const int y = std::clamp(
				static_cast<int>(std::lround(static_cast<float>(midY) - normalized * static_cast<float>(midY - 1))),
				0,
				OSCILLOSCOPE_HEIGHT - 1);
			rows[y][x] = '*';
		}

		constexpr int cursorX = OSCILLOSCOPE_WIDTH / 2;
		for (int y = 0; y < OSCILLOSCOPE_HEIGHT; ++y)
		{
			rows[y][cursorX] = (y == midY) ? '+' : '|';
		}

		std::string progressBar(OSCILLOSCOPE_WIDTH, '.');
		const float progress = composition.durationSeconds > 0.f
			? std::clamp(currentSeconds / composition.durationSeconds, 0.f, 1.f)
			: 0.f;
		const int filled = static_cast<int>(std::round(progress * static_cast<float>(OSCILLOSCOPE_WIDTH)));
		for (int i = 0; i < filled && i < OSCILLOSCOPE_WIDTH; ++i)
		{
			progressBar[i] = '#';
		}

		std::cout << "\x1b[2J\x1b[H";
		std::cout << "sound-player\n";
		std::cout << "Current time: " << currentSeconds << "s / " << composition.durationSeconds << "s\n";
		std::cout << progressBar << "\n";
		for (const std::string& row : rows)
		{
			std::cout << row << '\n';
		}
		std::cout.flush();

		if (currentSeconds >= composition.durationSeconds)
		{
			break;
		}

		std::this_thread::sleep_for(frameDelay);
	}
}
