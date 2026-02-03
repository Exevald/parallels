#include "TempFileManager.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace tempFileManager
{

std::string CreateTempFile()
{
	char tpl[] = "/tmp/makearchive_XXXXXX";
	int fd = mkstemp(tpl);
	if (fd == -1)
	{
		perror("mkstemp");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return { tpl };
}

void CleanupTempFiles(const std::vector<std::string>& tempFiles)
{
	for (const auto& path : tempFiles)
	{
		unlink(path.c_str());
	}
}

} // namespace tempFileManager