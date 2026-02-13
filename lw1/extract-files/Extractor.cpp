#include "Extractor.h"
#include "FileScanner.h"

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace
{

bool WaitForChild(pid_t pid)
{
	int status = 0;
	if (waitpid(pid, &status, 0) == -1)
	{
		perror("waitpid");
		return false;
	}

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		std::cerr << "Error: "
				  << "tar extraction"
				  << " failed (exit code " << WEXITSTATUS(status) << ")\n";
		return false;
	}

	return true;
}

} // namespace

bool extractor::ExtractFilesFromArchive(const std::string& archiveName, const std::string& outputFolder)
{
	pid_t pid = fork();
	if (pid == 0)
	{
		execlp("tar", "tar", "-xf", archiveName.c_str(), "-C", outputFolder.c_str(), nullptr);
		perror("execlp tar");
		_exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		perror("fork (tar)");
		return false;
	}

	return WaitForChild(pid);
}

std::vector<std::string> extractor::GetExtractedFiles(const std::string& outputFolder)
{
	return fileScanner::ListFilesInDirectory(outputFolder);
}
