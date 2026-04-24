#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace wavWriter
{
void Save(const std::string& path, const std::vector<std::int16_t>& samples);
}
