#include "read.h"
#include "csv_read.h"
#include "xlsx_read.h"
#include "util/str.h"

#include <filesystem>

namespace io {

bool readFile(const std::string& path, std::vector<Student>& out,
              std::string& err, bool& wasEmpty) {
    std::string ext = util::toLower(std::filesystem::path(path).extension().string());
    if (ext == ".csv")  return readCsv(path, out, err, wasEmpty);
    if (ext == ".xlsx") return readXlsx(path, out, err, wasEmpty);
    err = "Unsupported file type '" + ext + "' (expected .csv or .xlsx)";
    return false;
}

}