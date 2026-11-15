#pragma once
#include "core/student.h"

#include <string>
#include <vector>

namespace io {

bool writeXlsx(const std::string& path, const std::vector<Student>& list,
               std::string& err);

}