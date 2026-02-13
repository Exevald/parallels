#include "FileScanner.h"

#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

namespace fileScanner
{

std::vector<std::string> ListFilesInDirectory(const std::string& directory)
{
	std::vector<std::string> files;
	DIR* dir = opendir(directory.c_str());
	if (!dir)
	{
		perror(("opendir '" + directory + "'").c_str());
		return files;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
		{
			continue;
		}

		std::string fullPath = directory + "/" + entry->d_name;

		struct stat st;
		if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
		{
			files.push_back(fullPath);
		}
	}

	closedir(dir);
	return files;
}

} // namespace fileScanner