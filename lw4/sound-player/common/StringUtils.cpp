#include "common/StringUtils.h"

std::string Trim(const std::string_view text)
{
	const auto first = text.find_first_not_of(" \t\r\n");
	if (first == std::string_view::npos)
	{
		return "";
	}

	const auto last = text.find_last_not_of(" \t\r\n");
	return std::string(text.substr(first, last - first + 1));
}

std::vector<std::string> Split(std::string_view text, char delimiter)
{
	std::vector<std::string> parts;
	std::size_t start = 0;
	while (start <= text.size())
	{
		const auto next = text.find(delimiter, start);
		if (next == std::string_view::npos)
		{
			parts.emplace_back(text.substr(start));
			break;
		}

		parts.emplace_back(text.substr(start, next - start));
		start = next + 1;
	}

	return parts;
}
