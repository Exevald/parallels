#include "GpuRadixSort.h"

#include <chrono>
#include <limits>
#include <stdexcept>

namespace
{

constexpr auto KERNEL_SOURCE = R"CLC(
	__kernel void makeFlags(
		__global const int* input,
		__global uint* flags,
		uint count,
		uint bit)
	{
		uint index = get_global_id(0);
		if (index >= count)
		{
			return;
		}

		uint key = as_uint(input[index]) ^ 0x80000000u;
		flags[index] = ((key >> bit) & 1u) == 0u;
	}

	__kernel void scanBlocks(
		__global const uint* input,
		__global uint* output,
		__global uint* blockSums,
		__local uint* values,
		uint count)
	{
		uint localId = get_local_id(0);
		uint blockStart = get_group_id(0) * get_local_size(0) * 2;
		uint first = blockStart + localId;
		uint second = first + get_local_size(0);

		values[localId] = first < count ? input[first] : 0u;
		values[localId + get_local_size(0)] = second < count ? input[second] : 0u;

		uint offset = 1u;
		for (uint active = get_local_size(0); active > 0u; active >>= 1u)
		{
			barrier(CLK_LOCAL_MEM_FENCE);
			if (localId < active)
			{
				uint right = offset * (2u * localId + 2u) - 1u;
				uint left = right - offset;
				values[right] += values[left];
			}
			offset <<= 1u;
		}

		if (localId == 0u)
		{
			uint last = get_local_size(0) * 2u - 1u;
			blockSums[get_group_id(0)] = values[last];
			values[last] = 0u;
		}

		for (uint active = 1u; active <= get_local_size(0); active <<= 1u)
		{
			offset >>= 1u;
			barrier(CLK_LOCAL_MEM_FENCE);
			if (localId < active)
			{
				uint right = offset * (2u * localId + 2u) - 1u;
				uint left = right - offset;
				uint temporary = values[left];
				values[left] = values[right];
				values[right] += temporary;
			}
		}
		barrier(CLK_LOCAL_MEM_FENCE);

		if (first < count)
		{
			output[first] = values[localId];
		}
		if (second < count)
		{
			output[second] = values[localId + get_local_size(0)];
		}
	}

	__kernel void addOffsets(
		__global uint* values,
		__global const uint* blockOffsets,
		uint count)
	{
		uint index = get_global_id(0);
		if (index < count)
		{
			uint block = index / (get_local_size(0) * 2u);
			values[index] += blockOffsets[block];
		}
	}

	__kernel void writeTotal(
		__global const uint* flags,
		__global const uint* positions,
		__global uint* total,
		uint count)
	{
		if (get_global_id(0) == 0u)
		{
			total[0] = positions[count - 1u] + flags[count - 1u];
		}
	}

	__kernel void scatter(
		__global const int* input,
		__global int* output,
		__global const uint* flags,
		__global const uint* positions,
		__global const uint* totalZeros,
		uint count)
	{
		uint index = get_global_id(0);
		if (index >= count)
		{
			return;
		}

		uint zeroPosition = positions[index];
		uint destination = flags[index]
			? zeroPosition
			: totalZeros[0] + index - zeroPosition;
		output[destination] = input[index];
	}
)CLC";

std::size_t RoundUp(const std::size_t value, const std::size_t groupSize)
{
	return (value + groupSize - 1) / groupSize * groupSize;
}

} // namespace

GpuRadixSort::GpuRadixSort()
	: m_gpu(KERNEL_SOURCE)
	, m_makeFlags(m_gpu.CreateKernel("makeFlags"))
	, m_scanBlocks(m_gpu.CreateKernel("scanBlocks"))
	, m_addOffsets(m_gpu.CreateKernel("addOffsets"))
	, m_writeTotal(m_gpu.CreateKernel("writeTotal"))
	, m_scatter(m_gpu.CreateKernel("scatter"))
{
}

float GpuRadixSort::Sort(std::vector<std::int32_t>& values)
{
	if (values.empty())
	{
		return 0.0f;
	}
	if (values.size() > std::numeric_limits<cl_uint>::max())
	{
		throw std::length_error("The array is too large for this OpenCL implementation");
	}

	const auto start = std::chrono::steady_clock::now();
	const std::size_t count = values.size();
	const std::size_t valueBytes = count * sizeof(std::int32_t);
	const std::size_t indexBytes = count * sizeof(cl_uint);
	const cl_uint openClCount = static_cast<cl_uint>(count);
	cl::Buffer input = m_gpu.CreateBuffer(
		CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		valueBytes,
		values.data());
	cl::Buffer output = m_gpu.CreateBuffer(CL_MEM_READ_WRITE, valueBytes);
	const cl::Buffer flags = m_gpu.CreateBuffer(CL_MEM_READ_WRITE, indexBytes);
	const cl::Buffer positions = m_gpu.CreateBuffer(CL_MEM_READ_WRITE, indexBytes);
	const cl::Buffer totalZeros = m_gpu.CreateBuffer(CL_MEM_READ_WRITE, sizeof(cl_uint));
	const ScanBuffers scanBuffers = CreateScanBuffers(count);
	const cl::NDRange global(RoundUp(count, WORK_GROUP_SIZE));
	const cl::NDRange local(WORK_GROUP_SIZE);


	for (cl_uint bit = 0; bit < 32; ++bit)
	{
		m_gpu.SetArg(m_makeFlags, 0, input);
		m_gpu.SetArg(m_makeFlags, 1, flags);
		m_gpu.SetArg(m_makeFlags, 2, openClCount);
		m_gpu.SetArg(m_makeFlags, 3, bit);
		m_gpu.Run(m_makeFlags, global, local);

		Scan(flags, positions, count, scanBuffers, 0);

		m_gpu.SetArg(m_writeTotal, 0, flags);
		m_gpu.SetArg(m_writeTotal, 1, positions);
		m_gpu.SetArg(m_writeTotal, 2, totalZeros);
		m_gpu.SetArg(m_writeTotal, 3, openClCount);
		m_gpu.Run(m_writeTotal, cl::NDRange(1), cl::NDRange(1));

		m_gpu.SetArg(m_scatter, 0, input);
		m_gpu.SetArg(m_scatter, 1, output);
		m_gpu.SetArg(m_scatter, 2, flags);
		m_gpu.SetArg(m_scatter, 3, positions);
		m_gpu.SetArg(m_scatter, 4, totalZeros);
		m_gpu.SetArg(m_scatter, 5, openClCount);
		m_gpu.Run(m_scatter, global, local);
		std::swap(input, output);
	}

	m_gpu.Read(input, values.data(), valueBytes);
	return std::chrono::duration<float, std::milli>(
		std::chrono::steady_clock::now() - start).count();
}

std::string GpuRadixSort::GetDeviceName() const
{
	return m_gpu.GetDeviceName();
}

GpuRadixSort::ScanBuffers GpuRadixSort::CreateScanBuffers(std::size_t count) const
{
	ScanBuffers buffers;
	while (true)
	{
		const std::size_t groups = (count + ELEMENTS_PER_GROUP - 1) / ELEMENTS_PER_GROUP;
		buffers.sums.push_back(m_gpu.CreateBuffer(
			CL_MEM_READ_WRITE,
			groups * sizeof(cl_uint)));
		if (groups == 1)
		{
			break;
		}
		buffers.offsets.push_back(m_gpu.CreateBuffer(
			CL_MEM_READ_WRITE,
			groups * sizeof(cl_uint)));
		count = groups;
	}
	return buffers;
}

void GpuRadixSort::Scan(
	const cl::Buffer& input,
	const cl::Buffer& output,
	const std::size_t count,
	const ScanBuffers& buffers,
	const std::size_t level)
{
	const std::size_t groups = (count + ELEMENTS_PER_GROUP - 1) / ELEMENTS_PER_GROUP;
	const cl_uint openClCount = static_cast<cl_uint>(count);
	m_gpu.SetArg(m_scanBlocks, 0, input);
	m_gpu.SetArg(m_scanBlocks, 1, output);
	m_gpu.SetArg(m_scanBlocks, 2, buffers.sums[level]);
	m_gpu.SetLocalArg(m_scanBlocks, 3, ELEMENTS_PER_GROUP * sizeof(cl_uint));
	m_gpu.SetArg(m_scanBlocks, 4, openClCount);
	m_gpu.Run(
		m_scanBlocks,
		cl::NDRange(groups * WORK_GROUP_SIZE),
		cl::NDRange(WORK_GROUP_SIZE));

	if (groups == 1)
	{
		return;
	}

	Scan(buffers.sums[level], buffers.offsets[level], groups, buffers, level + 1);
	m_gpu.SetArg(m_addOffsets, 0, output);
	m_gpu.SetArg(m_addOffsets, 1, buffers.offsets[level]);
	m_gpu.SetArg(m_addOffsets, 2, openClCount);
	m_gpu.Run(
		m_addOffsets,
		cl::NDRange(RoundUp(count, WORK_GROUP_SIZE)),
		cl::NDRange(WORK_GROUP_SIZE));
}