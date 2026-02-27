#pragma once

#include <iosfwd>
#include <string>
#include <vector>

class Grid
{
public:
	Grid(int width, int height);
	explicit Grid(const std::string& filename);

	void LoadFromFile(const std::string& filename);
	void SaveToFile(const std::string& filename) const;

	void GenerateRandom(double probability);

	[[nodiscard]] int GetWidth() const { return m_width; }
	[[nodiscard]] int GetHeight() const { return m_height; }
	[[nodiscard]] const std::vector<std::vector<bool>>& GetGrid() const { return m_grid; }

	[[nodiscard]] int CountNeighbors(int x, int y) const;

	void SetCell(int x, int y, bool alive);
	[[nodiscard]] bool GetCell(int x, int y) const;

private:
	std::vector<std::vector<bool>> m_grid;
	int m_width{};
	int m_height{};
};