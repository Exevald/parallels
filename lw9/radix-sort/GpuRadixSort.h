#pragma once

#include <GpuRunner.h>

#include <string>
#include <vector>

class GpuRadixSort
{
public:
	GpuRadixSort();

	float Sort(std::vector<std::int32_t>& values);

	[[nodiscard]] std::string GetDeviceName() const;

private:
	static constexpr std::size_t WORK_GROUP_SIZE = 256;
	static constexpr std::size_t ELEMENTS_PER_GROUP = WORK_GROUP_SIZE * 2;

	struct ScanBuffers
	{
		std::vector<cl::Buffer> sums;
		std::vector<cl::Buffer> offsets;
	};

	[[nodiscard]] ScanBuffers CreateScanBuffers(std::size_t count) const;
	void Scan(
		const cl::Buffer& input,
		const cl::Buffer& output,
		std::size_t count,
		const ScanBuffers& buffers,
		std::size_t level);

	GpuRunner m_gpu;
	cl::Kernel m_makeFlags;
	cl::Kernel m_scanBlocks;
	cl::Kernel m_addOffsets;
	cl::Kernel m_writeTotal;
	cl::Kernel m_scatter;
};
