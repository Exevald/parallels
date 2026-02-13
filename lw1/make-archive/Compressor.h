#pragma once

#include <string>
#include <vector>
#include <map>

namespace compressor
{

struct CompressionResult
{
	std::vector<std::string> compressedFiles;
	std::vector<std::string> tempFiles;
};

bool WaitForChild(pid_t pid, const std::string& fileName);
CompressionResult CompressSequentially(const std::vector<std::string>& inputFiles);
CompressionResult CompressInParallel(const std::vector<std::string>& inputFiles, int maxProcesses);

} // namespace compressor