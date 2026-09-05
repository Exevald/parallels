#pragma once

#include <GpuRunner.h>

#include <SFML/Graphics/Image.hpp>
#include <string>
#include <vector>

class GpuGaussianBlur
{
public:
	static constexpr int MAX_RADIUS = 70;

	explicit GpuGaussianBlur(const sf::Image& image);

	void Apply(int radius);

	[[nodiscard]] const std::vector<std::uint8_t>& GetPixels() const;
	[[nodiscard]] sf::Vector2u GetSize() const;
	[[nodiscard]] float GetTimeMs() const;
	[[nodiscard]] std::string GetDeviceName() const;

private:
	static constexpr std::size_t WORK_GROUP_WIDTH = 16;
	static constexpr std::size_t WORK_GROUP_HEIGHT = 8;

	void CreateBuffers();
	void RunKernel(cl::Kernel& kernel, const cl::Buffer& input, const cl::Buffer& output) const;
	void RunBlur(
		const cl::Buffer& input,
		const cl::Buffer& output,
		int radius,
		const std::vector<float>& weights,
		bool horizontal);

	sf::Vector2u m_size;
	std::vector<std::uint8_t> m_source;
	std::vector<std::uint8_t> m_result;
	int m_maxStageRadius = 0;

	GpuRunner m_gpu;
	cl::Kernel m_decode;
	cl::Kernel m_blur;
	cl::Kernel m_encode;
	cl::Buffer m_sourceBuffer;
	cl::Buffer m_first;
	cl::Buffer m_second;
	cl::Buffer m_outputBuffer;
	float m_timeMs = 0.0f;
};
