#include "write.h"
#include "csv_write.h"
#include "xlsx_write.h"
#include "util/str.h"

#include <filesystem>

namespace io {

bool writeFile(const std::string& path, const std::vector<Student>& list,
               std::string& err) {
    std::string ext = util::toLower(std::filesystem::path(path).extension().string());
    if (ext == ".csv")  return writeCsv(path, list, err);
    if (ext == ".xlsx") return writeXlsx(path, list, err);
    err = "Unsupported file type '" + ext + "' (expected .csv or .xlsx)";
    return false;
}

}