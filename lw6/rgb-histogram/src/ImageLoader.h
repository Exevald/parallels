#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct RgbImage
{
	int width = 0;
	int height = 0;
	std::vector<uint8_t> pixels;

	[[nodiscard]] size_t PixelCount() const;
	[[nodiscard]] bool Empty() const;
};

namespace ImageLoader
{

RgbImage LoadRgbImage(const std::string& filename);
RgbImage GenerateSolidColor(int width, int height, uint8_t red, uint8_t green, uint8_t blue);
RgbImage GenerateGradient(int width, int height);
RgbImage GenerateRandomImage(int width, int height, uint32_t seed);

}; // namespace ImageLoader
