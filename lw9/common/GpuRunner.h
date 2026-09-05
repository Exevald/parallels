#pragma once

#include <opencl.hpp>
#include <string>

class GpuRunner
{
public:
	explicit GpuRunner(const char* source);

	[[nodiscard]] cl::Kernel CreateKernel(const char* name) const;
	[[nodiscard]] cl::Buffer CreateBuffer(
		cl_mem_flags flags,
		std::size_t size,
		void* data = nullptr) const;

	template <typename T>
	void SetArg(cl::Kernel& kernel, const cl_uint index, const T& value) const
	{
		kernel.setArg(index, value);
	}

	void SetLocalArg(cl::Kernel& kernel, cl_uint index, std::size_t size) const;
	void Run(const cl::Kernel& kernel, const cl::NDRange& global, const cl::NDRange& local) const;
	void Read(const cl::Buffer& buffer, void* destination, std::size_t size) const;

	[[nodiscard]] cl_ulong GetLocalMemorySize() const;
	[[nodiscard]] std::string GetDeviceName() const;

private:
	cl::Device m_device;
	cl::Context m_context;
	cl::CommandQueue m_queue;
	cl::Program m_program;
};
