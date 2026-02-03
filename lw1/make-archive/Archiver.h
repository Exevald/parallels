#pragma once

#include <string>
#include <vector>

namespace archiver
{

bool CreateTarArchive(const std::string& archiveName, const std::vector<std::string>& compressedFiles);

} // namespace archiver