#pragma once

#include "model/Types.h"

#include <string>

namespace melodyParser
{
Composition ParseFile(const std::string& path);
}
