#pragma once
#include <string>

namespace util {

std::string cleanPath(std::string s);
std::string expandTilde(std::string s);
std::string resolvePath(const std::string& raw);

}