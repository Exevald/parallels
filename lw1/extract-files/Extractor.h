#pragma once

#include <string>
#include <vector>

namespace extractor
{

bool ExtractFilesFromArchive(const std::string& archiveName, const std::string& outputFolder);
std::vector<std::string> GetExtractedFiles(const std::string& outputFolder);

}