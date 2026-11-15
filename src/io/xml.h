#pragma once
#include <string>

namespace io::xml {

std::string escape  (const std::string& s);
std::string unescape(const std::string& s);

std::string textCell(const std::string& ref, const std::string& s);
std::string numCell (const std::string& ref, double v);

std::string columnName(int zeroBasedCol);

}