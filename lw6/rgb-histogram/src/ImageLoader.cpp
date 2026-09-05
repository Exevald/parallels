#include "ImageLoader.h"
#include "StbiImageGuard.h"
#include "stb_image.h"

#include <random>
#include <stdexcept>

size_t RgbImage::PixelCount() const
{
	return static_cast<size_t>(width) * static_cast<size_t>(height);
}

bool RgbImage::Empty() const
{
	return width <= 0 || height <= 0 || pixels.empty();
}

RgbImage ImageLoader::LoadRgbImage(const std::string& filename)
{
	int width = 0;
	int height = 0;
	int channels = 0;
	const StbiImageGuard data(stbi_load(
		filename.c_str(),
		&width,
		&height,
		&channels,
		3));
	if (data.Get() == nullptr)
	{
		throw std::runtime_error("Failed to load image: " + filename);
	}

	RgbImage image;
	image.width = width;
	image.height = height;
	image.pixels.assign(data.Get(), data.Get() + image.PixelCount() * 3);
	return image;
}

RgbImage ImageLoader::GenerateSolidColor(
	const int width,
	const int height,
	const uint8_t red,
	const uint8_t green,
	const uint8_t blue)
{
	if (width <= 0 || height <= 0)
	{
		throw std::invalid_argument("Image dimensions must be positive");
	}

	RgbImage image;
	image.width = width;
	image.height = height;
	image.pixels.resize(image.PixelCount() * 3);

	for (size_t pixelIndex = 0; pixelIndex < image.PixelCount(); ++pixelIndex)
	{
		const size_t offset = pixelIndex * 3;
		image.pixels[offset + 0] = red;
		image.pixels[offset + 1] = green;
		image.pixels[offset + 2] = blue;
	}

	return image;
}

RgbImage ImageLoader::GenerateGradient(const int width, const int height)
{
	if (width <= 0 || height <= 0)
	{
		throw std::invalid_argument("Image dimensions must be positive");
	}

	RgbImage image;
	image.width = width;
	image.height = height;
	image.pixels.resize(image.PixelCount() * 3);

	const auto widthScale = static_cast<uint32_t>(width > 1 ? width - 1 : 1);
	const auto heightScale = static_cast<uint32_t>(height > 1 ? height - 1 : 1);
	const auto diagonalScale = static_cast<uint32_t>(width + height > 2 ? width + height - 2 : 1);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const size_t pixelIndex = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
			const size_t offset = pixelIndex * 3;
			image.pixels[offset + 0] = static_cast<uint8_t>((static_cast<uint32_t>(x) * 255U) / widthScale);
			image.pixels[offset + 1] = static_cast<uint8_t>((static_cast<uint32_t>(y) * 255U) / heightScale);
			image.pixels[offset + 2] = static_cast<uint8_t>((static_cast<uint32_t>(x + y) * 255U) / diagonalScale);
		}
	}

	return image;
}

RgbImage ImageLoader::GenerateRandomImage(const int width, const int height, const uint32_t seed)
{
	if (width <= 0 || height <= 0)
	{
		throw std::invalid_argument("Image dimensions must be positive");
	}

	RgbImage image;
	image.width = width;
	image.height = height;
	image.pixels.resize(image.PixelCount() * 3);

	std::mt19937 generator(seed);
	std::uniform_int_distribution distribution(0, 255);
	for (uint8_t& value : image.pixels)
	{
		value = static_cast<uint8_t>(distribution(generator));
	}

	return image;
}