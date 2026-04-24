#pragma once

#include <string>
#include <string_view>
#include <vector>

std::string Trim(std::string_view text);
std::vector<std::string> Split(std::string_view text, char delimiter);
