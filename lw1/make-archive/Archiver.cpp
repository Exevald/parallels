#include "Archiver.h"
#include "Compressor.h"

#include <unistd.h>
#include <vector>

namespace archiver
{

bool CreateTarArchive(const std::string& archiveName, const std::vector<std::string>& compressedFiles)
{
	pid_t pid = fork();
	if (pid == 0)
	{
		std::vector<char*> args;
		args.reserve(compressedFiles.size() + 4);
		args.push_back(const_cast<char*>("tar"));
		args.push_back(const_cast<char*>("-cf"));
		args.push_back(const_cast<char*>(archiveName.c_str()));
		for (const auto& file : compressedFiles)
		{
			args.push_back(const_cast<char*>(file.c_str()));
		}
		args.push_back(nullptr);

		execvp("tar", args.data());
		perror("execvp tar");
		_exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		perror("fork (tar)");
		return false;
	}

	return compressor::WaitForChild(pid, "tar archive creation");
}

} // namespace archiver