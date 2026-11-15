#pragma once
#include <string>

namespace cli {

int runShell   (const std::string& initialPath);
int runValidate(const std::string& path);
int runList    (const std::string& path);

}