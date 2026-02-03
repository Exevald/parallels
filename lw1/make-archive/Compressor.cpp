#include "Compressor.h"
#include "FileDesc.h"
#include "TempFileManager.h"

#include <iostream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace compressor
{

bool WaitForChild(pid_t pid, const std::string& fileName)
{
	int status = 0;
	if (waitpid(pid, &status, 0) == -1)
	{
		perror("waitpid");
		return false;
	}

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		std::cerr << "Error: gzip failed for '" << fileName << "'" << std::endl;
		return false;
	}

	return true;
}

std::vector<CompressionContext> StartCompressionBatch(
	const std::vector<std::string>& inputFiles,
	size_t startIndex,
	size_t count)
{
	std::vector<CompressionContext> contexts;
	contexts.reserve(count);

	for (size_t i = 0; i < count && startIndex + i < inputFiles.size(); ++i)
	{
		const auto& inputFile = inputFiles[startIndex + i];
		std::string tempPath = tempFileManager::CreateTempFile();

		pid_t pid = fork();
		if (pid == 0)
		{
			FileDesc outDesc(tempPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
			dup2(outDesc.GetDesc(), STDOUT_FILENO);
			outDesc.Close();

			execlp("gzip", "gzip", "-c", inputFile.c_str(), nullptr);
			perror("execlp gzip");
			_exit(EXIT_FAILURE);
		}
		else if (pid < 0)
		{
			perror("fork");
			tempFileManager::CleanupTempFiles({ tempPath });
			exit(EXIT_FAILURE);
		}

		contexts.push_back({ inputFile, tempPath, pid });
	}

	return contexts;
}

CompressionResult CompressSequentially(const std::vector<std::string>& inputFiles)
{
	CompressionResult result;
	result.compressedFiles.reserve(inputFiles.size());
	result.tempFiles.reserve(inputFiles.size());

	for (const auto& inputFile : inputFiles)
	{
		auto contexts = StartCompressionBatch({ inputFile }, 0, 1);
		const auto& ctx = contexts[0];

		result.tempFiles.push_back(ctx.tempFile);

		if (!WaitForChild(ctx.pid, inputFile))
		{
			tempFileManager::CleanupTempFiles(result.tempFiles);
			exit(EXIT_FAILURE);
		}

		result.compressedFiles.push_back(ctx.tempFile);
	}

	return result;
}

CompressionResult CompressInParallel(const std::vector<std::string>& inputFiles, int maxProcesses)
{
	CompressionResult result;
	result.compressedFiles.reserve(inputFiles.size());

	size_t nextIdx = 0;
	std::map<pid_t, CompressionContext> activeChildren;

	while (nextIdx < inputFiles.size() || !activeChildren.empty())
	{
		while (nextIdx < inputFiles.size() && static_cast<int>(activeChildren.size()) < maxProcesses)
		{
			auto batch = StartCompressionBatch(inputFiles, nextIdx, 1);
			const auto& ctx = batch[0];

			activeChildren[ctx.pid] = ctx;
			result.tempFiles.push_back(ctx.tempFile);
			++nextIdx;
		}

		int status = 0;
		pid_t pid = waitpid(-1, &status, 0);
		if (pid == -1)
		{
			perror("waitpid");
			tempFileManager::CleanupTempFiles(result.tempFiles);
			exit(EXIT_FAILURE);
		}

		auto it = activeChildren.find(pid);
		if (it == activeChildren.end())
		{
			continue;
		}

		const auto& ctx = it->second;
		activeChildren.erase(it);

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		{
			result.compressedFiles.push_back(ctx.tempFile);
		}
		else
		{
			std::cerr << "Error: gzip failed for '" << ctx.inputFile
					  << "' (exit code " << WEXITSTATUS(status) << ")" << std::endl;
			tempFileManager::CleanupTempFiles(result.tempFiles);
			exit(EXIT_FAILURE);
		}
	}

	return result;
}

} // namespace compressor