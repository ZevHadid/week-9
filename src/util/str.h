#pragma once
#include <string>

namespace util {

std::string toLower(std::string s);
std::string trim(const std::string& s);
std::string fmt(double value, int precision = 2);
bool        parseDouble(const std::string& raw, double& out);

}