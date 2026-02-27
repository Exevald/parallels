#include "LifeSimulator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <vector>

LifeSimulator::LifeSimulator(const std::string& filename)
	: m_grid(filename)
{
}

LifeSimulator::LifeSimulator(int width, int height)
	: m_grid(width, height)
{
}

double LifeSimulator::ComputeNextGeneration(size_t numThreads)
{
	if (numThreads == 0)
	{
		throw std::invalid_argument("Number of threads must be positive");
	}

	auto startTime = std::chrono::high_resolution_clock::now();

	std::vector<std::vector<bool>> nextGrid(
		m_grid.GetHeight(),
		std::vector<bool>(m_grid.GetWidth(), false));

	int height = m_grid.GetHeight();
	int width = m_grid.GetWidth();
	auto rowsPerThread = static_cast<size_t>(
		std::ceil(static_cast<double>(height) / static_cast<double>(numThreads)));

	std::vector<std::jthread> threads;
	threads.reserve(numThreads);

	for (size_t threadId = 0; threadId < numThreads; ++threadId)
	{
		size_t startRow = threadId * rowsPerThread;
		size_t endRow = std::min(startRow + rowsPerThread, static_cast<size_t>(height));
		if (startRow >= static_cast<size_t>(height))
		{
			continue;
		}

		threads.emplace_back([this, &nextGrid, startRow, endRow, width]() {
			for (size_t y = startRow; y < endRow; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					int neighbors = this->m_grid.CountNeighbors(x, static_cast<int>(y));
					bool isAlive = this->m_grid.GetCell(x, static_cast<int>(y));

					if (isAlive)
					{
						nextGrid[y][x] = (neighbors == 2 || neighbors == 3);
					}
					else
					{
						nextGrid[y][x] = (neighbors == 3);
					}
				}
			}
		});
	}

	int liveInNext = 0;
	for (auto& y : nextGrid)
	{
		for (auto&& x : y)
		{
			if (x)
			{
				liveInNext++;
			}
		}
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = endTime - startTime;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			m_grid.SetCell(x, y, nextGrid[y][x]);
		}
	}

	int liveAfter = 0;
	for (int y = 0; y < m_grid.GetHeight(); ++y)
	{
		for (int x = 0; x < m_grid.GetWidth(); ++x)
		{
			if (m_grid.GetCell(x, y))
			{
				liveAfter++;
			}
		}
	}

	return elapsed.count();
}

void LifeSimulator::SaveState(const std::string& filename) const
{
	m_grid.SaveToFile(filename);
}