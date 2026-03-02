#include "ImageLoader.h"
#include "deferrer.h"
#include "stb_image.h"
#include "stb_image_write.h"

bool ImageLoader::LoadImage(const std::string& filename)
{
	unsigned char* data = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 4);
	if (!data)
	{
		return false;
	}

	m_pixels.resize(m_width * m_height);
	for (int i = 0; i < m_width * m_height; ++i)
	{
		m_pixels[i].r = ToLinear(data[i * 4 + 0]);
		m_pixels[i].g = ToLinear(data[i * 4 + 1]);
		m_pixels[i].b = ToLinear(data[i * 4 + 2]);
		m_pixels[i].a = ToLinear(data[i * 4 + 3]);
	}

	deferrer stbiGuard([&]() {
		stbi_image_free(data);
	});

	return true;
}

bool ImageLoader::SaveImage(const std::string& filename, const std::vector<Pixel>& pixels, int width, int height)
{
	std::vector<unsigned char> outputBuffer(width * height * 4);
	for (int i = 0; i < width * height; ++i)
	{
		outputBuffer[i * 4 + 0] = ToGamma(pixels[i].r);
		outputBuffer[i * 4 + 1] = ToGamma(pixels[i].g);
		outputBuffer[i * 4 + 2] = ToGamma(pixels[i].b);
		outputBuffer[i * 4 + 3] = ToGamma(pixels[i].a);
	}
	return stbi_write_png(filename.c_str(), width, height, 4, outputBuffer.data(), width * 4) != 0;
}