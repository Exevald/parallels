#include "GaussianBlur.h"

#include <algorithm>
#include <cmath>
#include <thread>

GaussianBlur::GaussianBlur() = default;

void GaussianBlur::SetRadius(int radius)
{
	m_radius = radius;
}

void GaussianBlur::SetNumThreads(int numThreads)
{
	m_numThreads = (numThreads > 0) ? numThreads : 1;
}

void GaussianBlur::GenerateKernel()
{
	if (m_radius <= 0)
	{
		m_kernelWeights.clear();
		return;
	}

	int size = 2 * m_radius + 1;
	m_kernelWeights.resize(size);

	float sigma = static_cast<float>(m_radius) / 3.0f;
	float twoSigmaSq = 2.0f * sigma * sigma;
	float sum = 0.0f;

	for (int i = 0; i < size; ++i)
	{
		int x = i - m_radius;
		float value = std::exp(static_cast<float>(-(x * x)) / twoSigmaSq);
		m_kernelWeights[i] = value;
		sum += value;
	}
	for (int i = 0; i < size; ++i)
	{
		m_kernelWeights[i] /= sum;
	}
}

const Pixel& GaussianBlur::GetPixelClamp(
	const std::vector<Pixel>& image,
	int x, int y, int width, int height)
{
	x = std::clamp(x, 0, width - 1);
	y = std::clamp(y, 0, height - 1);
	return image[y * width + x];
}

void GaussianBlur::ApplyHorizontalPass(
	const std::vector<Pixel>& input,
	std::vector<Pixel>& output,
	int width, int height, int startRow, int endRow)
{
	if (m_radius == 0)
	{
		for (int i = startRow * width; i < endRow * width; ++i)
		{
			output[i] = input[i];
		}
		return;
	}

	for (int y = startRow; y < endRow; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			double r = 0, g = 0, b = 0, a = 0;
			for (size_t k = 0; k < m_kernelWeights.size(); ++k)
			{
				int sampleX = x - m_radius + static_cast<int>(k);
				const Pixel& p = GetPixelClamp(input, sampleX, y, width, height);
				float w = m_kernelWeights[k];
				r += p.r * w;
				g += p.g * w;
				b += p.b * w;
				a += p.a * w;
			}
			output[y * width + x] = { r, g, b, a };
		}
	}
}

void GaussianBlur::ApplyVerticalPass(
	const std::vector<Pixel>& input,
	std::vector<Pixel>& output,
	int width, int height, int startRow, int endRow)
{
	if (m_radius == 0)
	{
		for (int i = startRow * width; i < endRow * width; ++i)
		{
			output[i] = input[i];
		}
		return;
	}

	for (int y = startRow; y < endRow; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			double r = 0, g = 0, b = 0, a = 0;
			for (size_t k = 0; k < m_kernelWeights.size(); ++k)
			{
				int sampleY = y - m_radius + static_cast<int>(k);
				const Pixel& p = GetPixelClamp(input, x, sampleY, width, height);
				float w = m_kernelWeights[k];
				r += p.r * w;
				g += p.g * w;
				b += p.b * w;
				a += p.a * w;
			}
			output[y * width + x] = { r, g, b, a };
		}
	}
}

void GaussianBlur::ProcessHorizontalThread(
	const std::vector<Pixel>& input,
	std::vector<Pixel>& output,
	int width, int height, int startRow, int endRow)
{
	ApplyHorizontalPass(input, output, width, height, startRow, endRow);
}

void GaussianBlur::ProcessVerticalThread(
	const std::vector<Pixel>& input,
	std::vector<Pixel>& output,
	int width, int height, int startRow, int endRow)
{
	ApplyVerticalPass(input, output, width, height, startRow, endRow);
}

void GaussianBlur::Process(std::vector<Pixel>& image, int width, int height)
{
	if (image.empty() || m_radius <= 0)
	{
		return;
	}

	std::vector<Pixel> tempBuffer(width * height);

	ProcessWithThreads(image, tempBuffer, width, height, true);
	ProcessWithThreads(tempBuffer, image, width, height, false);
}

void GaussianBlur::ProcessWithThreads(
	const std::vector<Pixel>& input,
	std::vector<Pixel>& output,
	int width, int height,
	bool horizontal)
{
	std::vector<std::jthread> threads;
	int rowsPerThread = (height + m_numThreads - 1) / m_numThreads;

	for (int t = 0; t < m_numThreads; ++t)
	{
		int startRow = t * rowsPerThread;
		int endRow = std::min((t + 1) * rowsPerThread, height);

		if (startRow >= height)
		{
			break;
		}

		threads.emplace_back([this, &input, &output, width, height, startRow, endRow, horizontal]() {
			if (horizontal)
			{
				ProcessHorizontalThread(input, output, width, height, startRow, endRow);
			}
			else
			{
				ProcessVerticalThread(input, output, width, height, startRow, endRow);
			}
		});
	}
}