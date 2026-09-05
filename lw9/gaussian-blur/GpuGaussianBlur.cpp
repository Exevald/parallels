#include "GpuGaussianBlur.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace
{
constexpr auto KERNEL_SOURCE = R"CLC(
	float toLinear(float value)
	{
		return value <= 0.04045f
			? value / 12.92f
			: pow((value + 0.055f) / 1.055f, 2.4f);
	}

	float toSRGB(float value)
	{
		value = fmax(value, 0.0f);
		return value <= 0.0031308f
			? value * 12.92f
			: 1.055f * pow(value, 1.0f / 2.4f) - 0.055f;
	}

	__kernel void decode(
		__global const uchar4* input,
		__global float4* output,
		int width,
		int height)
	{
		int x = get_global_id(0);
		int y = get_global_id(1);
		if (x >= width || y >= height)
		{
			return;
		}

		int index = y * width + x;
		float4 color = convert_float4(input[index]) / 255.0f;
		output[index] = (float4)(toLinear(color.x), toLinear(color.y), toLinear(color.z), color.w);
	}

	__kernel void blur(
		__global const float4* input,
		__global float4* output,
		__constant const float* weights,
		__local float4* tile,
		__local float* localWeights,
		int width,
		int height,
		int radius,
		int horizontal)
	{
		int lx = get_local_id(0);
		int ly = get_local_id(1);
		int lw = get_local_size(0);
		int lh = get_local_size(1);
		int gx = get_group_id(0) * lw;
		int gy = get_group_id(1) * lh;
		int localId = ly * lw + lx;
		int localCount = lw * lh;
		int tileWidth = horizontal ? lw + 2 * radius : lw;
		int tileHeight = horizontal ? lh : lh + 2 * radius;

		// состояние гонки
		for (int i = localId; i <= radius; i += localCount)
		{
			localWeights[i] = weights[i];
		}

		for (int i = localId; i < tileWidth * tileHeight; i += localCount)
		{
			int tx = i % tileWidth;
			int ty = i / tileWidth;
			int x = clamp(gx + tx - (horizontal ? radius : 0), 0, width - 1);
			int y = clamp(gy + ty - (horizontal ? 0 : radius), 0, height - 1);
			tile[i] = input[y * width + x];
		}
		barrier(CLK_LOCAL_MEM_FENCE);

		int x = gx + lx;
		int y = gy + ly;
		if (x >= width || y >= height)
		{
			return;
		}

		int center = horizontal
			? ly * tileWidth + lx + radius
			: (ly + radius) * tileWidth + lx;
		int step = horizontal ? 1 : tileWidth;
		float4 sum = tile[center] * localWeights[0];
		for (int i = 1; i <= radius; ++i)
		{
			sum += (tile[center - i * step] + tile[center + i * step]) * localWeights[i];
		}
		output[y * width + x] = sum;
	}

	__kernel void encode(
		__global const float4* input,
		__global uchar4* output,
		int width,
		int height)
	{
		int x = get_global_id(0);
		int y = get_global_id(1);
		if (x >= width || y >= height)
		{
			return;
		}

		int index = y * width + x;
		float4 color = input[index];
		color = clamp((float4)(toSRGB(color.x), toSRGB(color.y), toSRGB(color.z), color.w), 0.0f, 1.0f);
		output[index] = convert_uchar4_sat_rte(color * 255.0f);
	}
)CLC";

std::size_t RoundUp(const std::size_t value, const std::size_t group)
{
	return (value + group - 1) / group * group;
}

std::vector<float> MakeKernel(const int radius, const float sigma)
{
	std::vector<float> weights(static_cast<std::size_t>(radius) + 1);
	float sum = 0.0f;
	for (int i = 0; i <= radius; ++i)
	{
		weights[static_cast<std::size_t>(i)] = std::exp(-static_cast<float>(i * i) / (2.0f * sigma * sigma));
		sum += weights[static_cast<std::size_t>(i)] * (i == 0 ? 1.0f : 2.0f);
	}
	for (float& weight : weights)
		weight /= sum;
	return weights;
}

} // namespace

GpuGaussianBlur::GpuGaussianBlur(const sf::Image& image)
	: m_size(image.getSize())
	, m_gpu(KERNEL_SOURCE)
	, m_decode(m_gpu.CreateKernel("decode"))
	, m_blur(m_gpu.CreateKernel("blur"))
	, m_encode(m_gpu.CreateKernel("encode"))
{
	if (m_size.x == 0 || m_size.y == 0 || image.getPixelsPtr() == nullptr)
	{
		throw std::invalid_argument("Image is empty");
	}

	const std::size_t bytes = static_cast<std::size_t>(m_size.x) * m_size.y * 4;
	m_source.assign(image.getPixelsPtr(), image.getPixelsPtr() + bytes);
	m_result = m_source;
	CreateBuffers();

	const cl_ulong localMemory = m_gpu.GetLocalMemorySize();
	for (int radius = MAX_RADIUS; radius > 0; --radius)
	{
		const std::size_t horizontal = (WORK_GROUP_WIDTH + 2 * radius) * WORK_GROUP_HEIGHT * sizeof(cl_float4);
		const std::size_t vertical = WORK_GROUP_WIDTH * (WORK_GROUP_HEIGHT + 2 * radius) * sizeof(cl_float4);
		if (const std::size_t weights = static_cast<std::size_t>(radius + 1) * sizeof(float);
			std::max(horizontal, vertical) + weights <= localMemory)
		{
			m_maxStageRadius = radius;
			break;
		}
	}
	if (m_maxStageRadius < 1)
	{
		throw std::runtime_error("Not enough OpenCL local memory");
	}
}

void GpuGaussianBlur::CreateBuffers()
{
	const std::size_t pixels = static_cast<std::size_t>(m_size.x) * m_size.y;
	const std::size_t rgbaBytes = pixels * 4;
	const std::size_t floatBytes = pixels * sizeof(cl_float4);

	m_sourceBuffer = m_gpu.CreateBuffer(
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, rgbaBytes, m_source.data());
	m_first = m_gpu.CreateBuffer(CL_MEM_READ_WRITE, floatBytes);
	m_second = m_gpu.CreateBuffer(CL_MEM_READ_WRITE, floatBytes);
	m_outputBuffer = m_gpu.CreateBuffer(CL_MEM_WRITE_ONLY, rgbaBytes);
}

void GpuGaussianBlur::RunKernel(
	cl::Kernel& kernel,
	const cl::Buffer& input,
	const cl::Buffer& output) const
{
	const int width = static_cast<int>(m_size.x);
	const int height = static_cast<int>(m_size.y);
	m_gpu.SetArg(kernel, 0, input);
	m_gpu.SetArg(kernel, 1, output);
	m_gpu.SetArg(kernel, 2, width);
	m_gpu.SetArg(kernel, 3, height);

	const cl::NDRange global(
		RoundUp(m_size.x, WORK_GROUP_WIDTH),
		RoundUp(m_size.y, WORK_GROUP_HEIGHT));
	const cl::NDRange local(WORK_GROUP_WIDTH, WORK_GROUP_HEIGHT);
	m_gpu.Run(kernel, global, local);
}

void GpuGaussianBlur::RunBlur(
	const cl::Buffer& input,
	const cl::Buffer& output,
	const int radius,
	const std::vector<float>& weights,
	const bool horizontal)
{
	const std::size_t weightsBytes = weights.size() * sizeof(float);
	const std::size_t tilePixels = horizontal
		? (WORK_GROUP_WIDTH + 2 * radius) * WORK_GROUP_HEIGHT
		: WORK_GROUP_WIDTH * (WORK_GROUP_HEIGHT + 2 * radius);
	const cl::Buffer weightsBuffer = m_gpu.CreateBuffer(
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		weightsBytes,
		const_cast<float*>(weights.data()));

	const int width = static_cast<int>(m_size.x);
	const int height = static_cast<int>(m_size.y);
	const int direction = horizontal ? 1 : 0;
	m_gpu.SetArg(m_blur, 0, input);
	m_gpu.SetArg(m_blur, 1, output);
	m_gpu.SetArg(m_blur, 2, weightsBuffer);
	m_gpu.SetLocalArg(m_blur, 3, tilePixels * sizeof(cl_float4));
	m_gpu.SetLocalArg(m_blur, 4, weightsBytes);
	m_gpu.SetArg(m_blur, 5, width);
	m_gpu.SetArg(m_blur, 6, height);
	m_gpu.SetArg(m_blur, 7, radius);
	m_gpu.SetArg(m_blur, 8, direction);

	const cl::NDRange global(
		RoundUp(m_size.x, WORK_GROUP_WIDTH),
		RoundUp(m_size.y, WORK_GROUP_HEIGHT));
	const cl::NDRange local(WORK_GROUP_WIDTH, WORK_GROUP_HEIGHT);
	m_gpu.Run(m_blur, global, local);
}

void GpuGaussianBlur::Apply(const int radius)
{
	if (radius < 0 || radius > MAX_RADIUS)
	{
		throw std::out_of_range("Invalid radius");
	}
	if (radius == 0)
	{
		m_result = m_source;
		m_timeMs = 0.0f;
		return;
	}

	const auto start = std::chrono::steady_clock::now();
	RunKernel(m_decode, m_sourceBuffer, m_first);

	const int stages = std::max(1, static_cast<int>(std::ceil(std::pow(static_cast<float>(radius) / m_maxStageRadius, 2.0f))));
	const float sigma = std::max(0.5f, radius / 3.0f) / std::sqrt(static_cast<float>(stages));
	const int stageRadius = std::min(m_maxStageRadius, std::max(1, static_cast<int>(std::ceil(3 * sigma))));
	const std::vector<float> weights = MakeKernel(stageRadius, sigma);

	for (int stage = 0; stage < stages; ++stage)
	{
		RunBlur(m_first, m_second, stageRadius, weights, true);
		RunBlur(m_second, m_first, stageRadius, weights, false);
	}

	RunKernel(m_encode, m_first, m_outputBuffer);
	m_gpu.Read(m_outputBuffer, m_result.data(), m_result.size());
	m_timeMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
}

const std::vector<std::uint8_t>& GpuGaussianBlur::GetPixels() const
{
	return m_result;
}

sf::Vector2u GpuGaussianBlur::GetSize() const
{
	return m_size;
}

float GpuGaussianBlur::GetTimeMs() const
{
	return m_timeMs;
}

std::string GpuGaussianBlur::GetDeviceName() const
{
	return m_gpu.GetDeviceName();
}