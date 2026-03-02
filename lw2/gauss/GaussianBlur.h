#pragma once

#include "ImageLoader.h"

#include <vector>

class GaussianBlur
{
public:
	GaussianBlur();

	void SetRadius(int radius);
	void SetNumThreads(int numThreads);
	void GenerateKernel();

	void Process(std::vector<Pixel>& image, int width, int height);

private:
	[[nodiscard]] static const Pixel& GetPixelClamp(
		const std::vector<Pixel>& image,
		int x, int y, int width, int height) ;

	void ApplyHorizontalPass(
		const std::vector<Pixel>& input,
		std::vector<Pixel>& output,
		int width, int height, int startRow, int endRow);
	void ApplyVerticalPass(
		const std::vector<Pixel>& input,
		std::vector<Pixel>& output,
		int width, int height, int startRow, int endRow);

	void ProcessHorizontalThread(
		const std::vector<Pixel>& input,
		std::vector<Pixel>& output,
		int width, int height, int startRow, int endRow);
	void ProcessVerticalThread(
		const std::vector<Pixel>& input,
		std::vector<Pixel>& output,
		int width, int height, int startRow, int endRow);
	void ProcessWithThreads(const std::vector<Pixel>& input, std::vector<Pixel>& output,
		int width, int height, bool horizontal);

	int m_radius{};
	int m_numThreads{ 1 };
	std::vector<float> m_kernelWeights;
};
