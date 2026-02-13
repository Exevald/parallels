#include "Decompressor.h"
#include "FileDesc.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace
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
		std::cerr << "Error: gzip failed to decompress '" << fileName << "'\n";
		return false;
	}

	return true;
}

std::string CreateTempPath(const std::string& originalFilename)
{
	char tempPath[1024];
	snprintf(tempPath, sizeof(tempPath), "%s.XXXXXX", originalFilename.c_str());
	int desc = mkstemp(tempPath);
	if (desc == -1)
	{
		perror("mkstemp");
		exit(EXIT_FAILURE);
	}
	close(desc);
	return { tempPath };
}

bool DecompressFile(const std::string& CompressedFile)
{
	std::string tempPath = CreateTempPath(CompressedFile);

	pid_t pid = fork();
	if (pid == 0)
	{
		FileDesc outDesc = FileDesc(tempPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (outDesc.GetDesc() == -1)
		{
			perror("open (output)");
			_exit(EXIT_FAILURE);
		}
		dup2(outDesc.GetDesc(), STDOUT_FILENO);
		outDesc.Close();

		FileDesc inDesc = FileDesc(CompressedFile.c_str(), O_RDONLY);
		if (inDesc.GetDesc() == -1)
		{
			perror("open (input)");
			_exit(EXIT_FAILURE);
		}
		dup2(inDesc.GetDesc(), STDIN_FILENO);
		inDesc.Close();

		execlp("gzip", "gzip", "-d", "-c", nullptr);
		perror("execlp gzip");
		_exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		perror("fork");
		return false;
	}

	if (!WaitForChild(pid, CompressedFile))
	{
		return false;
	}

	if (rename(tempPath.c_str(), CompressedFile.c_str()) != 0)
	{
		perror("rename");
		return false;
	}

	return true;
}

} // namespace

bool decompressor::DecompressSequentially(const std::vector<std::string>& compressedFiles)
{
	for (const auto& file : compressedFiles)
	{
		if (!DecompressFile(file))
		{
			return false;
		}
	}
	return true;
}

bool decompressor::DecompressInParallel(const std::vector<std::string>& compressedFiles, int maxProcesses)
{
	size_t nextIdx = 0;
	std::map<pid_t, std::pair<std::string, std::string>> activeChildren;

	while (nextIdx < compressedFiles.size() || !activeChildren.empty())
	{
		while (nextIdx < compressedFiles.size() && static_cast<int>(activeChildren.size()) < maxProcesses)
		{
			const auto& file = compressedFiles[nextIdx];
			std::string tempPath = CreateTempPath(file);

			pid_t pid = fork();
			if (pid == 0)
			{
				auto outDesc = FileDesc(tempPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (outDesc.GetDesc() == -1)
				{
					_exit(EXIT_FAILURE);
				}
				dup2(outDesc.GetDesc(), STDOUT_FILENO);
				outDesc.Close();

				auto inDesc = FileDesc(file.c_str(), O_RDONLY);
				if (inDesc.GetDesc() == -1)
				{
					_exit(EXIT_FAILURE);
				}
				dup2(inDesc.GetDesc(), STDIN_FILENO);
				inDesc.Close();

				execlp("gzip", "gzip", "-d", "-c", nullptr);
				_exit(EXIT_FAILURE);
			}
			else if (pid < 0)
			{
				perror("fork");
				return false;
			}

			activeChildren[pid] = { file, tempPath };
			++nextIdx;
		}

		int status;
		pid_t pid = waitpid(-1, &status, 0);
		if (pid == -1)
		{
			perror("waitpid");
			return false;
		}
		auto child = activeChildren.find(pid);
		if (child == activeChildren.end())
		{
			continue;
		}

		const auto& [originalFile, tempFile] = child->second;
		activeChildren.erase(child);

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		{
			std::cerr << "Error: gzip failed for '" << originalFile << "'\n";
			return false;
		}

		if (rename(tempFile.c_str(), originalFile.c_str()) != 0)
		{
			perror("rename");
			return false;
		}
	}

	return true;
}