#pragma once

#include "Grid.h"

#include <chrono>
#include <stop_token>
#include <thread>
#include <vector>

class LifeSimulator
{

public:
	explicit LifeSimulator(const std::string& filename);
	explicit LifeSimulator(int width, int height);

	double ComputeNextGeneration(size_t numThreads);

	Grid& GetGrid() { return m_grid; }

	void SaveState(const std::string& filename) const;

private:
	Grid m_grid;
};