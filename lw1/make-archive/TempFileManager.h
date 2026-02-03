#pragma once

#include <string>
#include <vector>

namespace tempFileManager
{

std::string CreateTempFile();
void CleanupTempFiles(const std::vector<std::string>& tempFiles);

} // namespace tempFileManager