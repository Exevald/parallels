#include "Grid.h"

#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

Grid::Grid(int w, int h)
	: m_width(w)
	, m_height(h)
{
	if (m_width <= 0 || m_height <= 0)
	{
		throw std::invalid_argument("Grid dimensions must be positive");
	}
	m_grid.resize(m_height, std::vector<bool>(m_width, false));
}

Grid::Grid(const std::string& filename)
{
	LoadFromFile(filename);
}

void Grid::LoadFromFile(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Cannot open file: " + filename);
	}

	file >> m_width >> m_height;
	if (m_width <= 0 || m_height <= 0)
	{
		throw std::runtime_error("Invalid grid dimensions in file: "
			+ std::to_string(m_width) + "x" + std::to_string(m_height));
	}

	m_grid.clear();
	m_grid.resize(m_height, std::vector<bool>(m_width, false));

	int linesRead = 0;
	for (int y = 0; y < m_height; ++y)
	{
		std::string line;
		if (!std::getline(file, line))
		{
			break;
		}
		linesRead++;
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		for (int x = 0; x < m_width && x < static_cast<int>(line.size()); ++x)
		{
			if (line[static_cast<size_t>(x)] == '#')
			{
				m_grid[y][x] = true;
			}
		}
	}
}

void Grid::SaveToFile(const std::string& filename) const
{
	std::ofstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Cannot open file for writing: " + filename);
	}

	file << m_width << " " << m_height << "\n";
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			file << (m_grid[y][x] ? '#' : ' ');
		}
		file << "\n";
	}
}

void Grid::GenerateRandom(double probability)
{
	if (probability < 0.0 || probability > 1.0)
	{
		throw std::invalid_argument("Probability must be in [0.0, 1.0]");
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);

	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			m_grid[y][x] = (dis(gen) < probability);
		}
	}
}

int Grid::CountNeighbors(int x, int y) const
{
	int count = 0;

	for (int dy = -1; dy <= 1; ++dy)
	{
		for (int dx = -1; dx <= 1; ++dx)
		{
			if (dx == 0 && dy == 0)
			{
				continue;
			}

			int neighborX = ((x + dx) % m_width + m_width) % m_width;
			int neighborY = ((y + dy) % m_height + m_height) % m_height;
			if (neighborX == x && neighborY == y)
			{
				continue;
			}

			if (m_grid[neighborY][neighborX])
			{
				count++;
			}
		}
	}

	return count;
}

void Grid::SetCell(int x, int y, bool alive)
{
	if (x >= 0 && x < m_width && y >= 0 && y < m_height)
	{
		m_grid[y][x] = alive;
	}
}

bool Grid::GetCell(int x, int y) const
{
	if (x >= 0 && x < m_width && y >= 0 && y < m_height)
	{
		return m_grid[y][x];
	}
	return false;
}