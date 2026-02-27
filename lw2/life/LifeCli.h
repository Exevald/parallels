#pragma once

#include <string>
#include <vector>

class LifeCli {
public:
	static int RunGenerateMode(const std::vector<std::string>& args);
	static int RunStepMode(const std::vector<std::string>& args);
	static void ShowUsage();
};