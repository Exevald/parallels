#include "Compressor.h"
#include "FileDesc.h"
#include "TempFileManager.h"

#include <fcntl.h>
#include <iostream>
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

CompressionResult CompressSequentially(const std::vector<std::string>& inputFiles)
{
	CompressionResult result;
	result.compressedFiles.reserve(inputFiles.size());
	result.tempFiles.reserve(inputFiles.size());

	for (const auto& inputFile : inputFiles)
	{
		std::string tempPath = tempFileManager::CreateTempFile();
		result.tempFiles.push_back(tempPath);

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
			tempFileManager::CleanupTempFiles(result.tempFiles);
			exit(EXIT_FAILURE);
		}

		if (!WaitForChild(pid, inputFile))
		{
			tempFileManager::CleanupTempFiles(result.tempFiles);
			exit(EXIT_FAILURE);
		}

		result.compressedFiles.push_back(tempPath);
	}

	return result;
}

CompressionResult CompressInParallel(const std::vector<std::string>& inputFiles, int maxProcesses)
{
	CompressionResult result;
	result.compressedFiles.reserve(inputFiles.size());

	size_t nextIdx = 0;
	std::map<pid_t, std::string> pidToInputFile;
	std::map<pid_t, std::string> pidToTempFile;

	while (nextIdx < inputFiles.size() || !pidToInputFile.empty())
	{
		while (nextIdx < inputFiles.size() && static_cast<int>(pidToInputFile.size()) < maxProcesses)
		{
			const std::string& inputFile = inputFiles[nextIdx];
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
				tempFileManager::CleanupTempFiles(result.tempFiles);
				exit(EXIT_FAILURE);
			}

			pidToInputFile[pid] = inputFile;
			pidToTempFile[pid] = tempPath;
			result.tempFiles.push_back(tempPath);
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

		auto itInput = pidToInputFile.find(pid);
		auto itTemp = pidToTempFile.find(pid);
		if (itInput == pidToInputFile.end() || itTemp == pidToTempFile.end())
		{
			continue;
		}

		const std::string inputFile = itInput->second;
		const std::string tempFile = itTemp->second;

		pidToInputFile.erase(itInput);
		pidToTempFile.erase(itTemp);

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		{
			result.compressedFiles.push_back(tempFile);
		}
		else
		{
			std::cerr << "Error: gzip failed for '" << inputFile
					  << "' (exit code " << WEXITSTATUS(status) << ")" << std::endl;
			tempFileManager::CleanupTempFiles(result.tempFiles);
			exit(EXIT_FAILURE);
		}
	}

	return result;
}

} // namespace compressor