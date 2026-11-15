#pragma once
#include "core/student.h"
#include <string>
#include <vector>

namespace io {
bool readFile(const std::string& path, std::vector<Student>& out,
              std::string& err, bool& wasEmpty);
}