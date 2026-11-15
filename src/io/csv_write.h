#pragma once
#include "core/student.h"
#include <string>
#include <vector>

namespace io {
bool writeCsv(const std::string& path, const std::vector<Student>& list,
              std::string& err);
}