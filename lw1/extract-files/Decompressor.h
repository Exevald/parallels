#pragma once

#include <string>
#include <vector>
#include <map>

namespace decompressor
{

bool DecompressSequentially(const std::vector<std::string>& compressedFiles);
bool DecompressInParallel(const std::vector<std::string>& compressedFiles, int maxProcesses);

} // namespace decompressor