#include "GpuRunner.h"

#include <stdexcept>
#include <vector>

namespace
{

cl::Device SelectGpu()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	for (const cl::Platform& platform : platforms)
	{
		std::vector<cl::Device> devices;
		try
		{
			platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
		}
		catch (const cl::Error& error)
		{
			if (error.err() != CL_DEVICE_NOT_FOUND)
			{
				throw;
			}
		}
		if (!devices.empty())
		{
			return devices.front();
		}
	}
	throw std::runtime_error("OpenCL GPU was not found");
}

} // namespace

GpuRunner::GpuRunner(const char* source)
	: m_device(SelectGpu())
	, m_context(m_device)
	, m_queue(m_context, m_device)
	, m_program(m_context, source)
{
	try
	{
		m_program.build({ m_device }, "-cl-std=CL1.2");
	}
	catch (const cl::BuildError&)
	{
		throw std::runtime_error("OpenCL build error:\n" + m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device));
	}
}

cl::Kernel GpuRunner::CreateKernel(const char* name) const
{
	return { m_program, name };
}

cl::Buffer GpuRunner::CreateBuffer(
	const cl_mem_flags flags,
	const std::size_t size,
	void* data) const
{
	return { m_context, flags, size, data };
}

void GpuRunner::SetLocalArg(
	cl::Kernel& kernel,
	const cl_uint index,
	const std::size_t size) const
{
	kernel.setArg(index, cl::Local(size));
}

void GpuRunner::Run(
	const cl::Kernel& kernel,
	const cl::NDRange& global,
	const cl::NDRange& local) const
{
	m_queue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
}

void GpuRunner::Read(
	const cl::Buffer& buffer,
	void* destination,
	const std::size_t size) const
{
	m_queue.enqueueReadBuffer(buffer, CL_TRUE, 0, size, destination);
}

cl_ulong GpuRunner::GetLocalMemorySize() const
{
	return m_device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
}

std::string GpuRunner::GetDeviceName() const
{
	return m_device.getInfo<CL_DEVICE_NAME>();
}
