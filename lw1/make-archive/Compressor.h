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

struct CompressionContext
{
	std::string inputFile;
	std::string tempFile;
	pid_t pid = -1;
};

bool WaitForChild(pid_t pid, const std::string& fileName);
std::vector<CompressionContext> StartCompressionBatch(
	const std::vector<std::string>& inputFiles,
	size_t startIndex,
	size_t count);

CompressionResult CompressSequentially(const std::vector<std::string>& inputFiles);
CompressionResult CompressInParallel(const std::vector<std::string>& inputFiles, int maxProcesses);

} // namespace compressor