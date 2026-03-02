#pragma once

#include "GammaCorrection.h"

#include <string>
#include <vector>

struct Pixel
{
	double r{}, g{}, b{}, a{};
};

class ImageLoader
{
public:
	ImageLoader() = default;

	bool LoadImage(const std::string& filename);
	static bool SaveImage(const std::string& filename, const std::vector<Pixel>& pixels, int width, int height);

	[[nodiscard]] int GetWidth() const { return m_width; }
	[[nodiscard]] int GetHeight() const { return m_height; }
	[[nodiscard]] int GetChannels() const { return m_channels; }

	std::vector<Pixel>& GetData() { return m_pixels; }
	[[nodiscard]] const std::vector<Pixel>& GetData() const { return m_pixels; }

private:
	int m_width{};
	int m_height{};
	int m_channels{};
	std::vector<Pixel> m_pixels;
};