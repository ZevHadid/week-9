#pragma once
#include <string>
#include <vector>

namespace io::zip {

struct Entry { std::string name, data; };

std::string build(const std::vector<Entry>& entries);
bool        extract(const std::string& archive, const std::string& entryName,
                    std::string& out);

}