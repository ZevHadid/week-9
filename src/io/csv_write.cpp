#include "csv_write.h"
#include "util/str.h"

#include <fstream>

namespace io {

namespace {

std::string escapeField(std::string s) {
    if (!s.empty() && (s[0] == '=' || s[0] == '+' || s[0] == '-' || s[0] == '@'))
        s = "'" + s;
    bool needsQuote = s.find_first_of(",\"\r\n") != std::string::npos;
    if (!needsQuote) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    return out + "\"";
}

}

bool writeCsv(const std::string& path, const std::vector<Student>& list,
              std::string& err) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        err = "Could not open \"" + path + "\" for writing.";
        return false;
    }
    f << "\xEF\xBB\xBF";

    auto headers = studentColumnHeaders();
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) f << ',';
        f << escapeField(headers[i]);
    }
    f << "\r\n";

    for (const auto& s : list) {
        f << escapeField(s.grade_year) << ','
          << escapeField(s.birth_date) << ','
          << escapeField(s.full_name) << ','
          << escapeField(s.gender);
        for (size_t j = 0; j < SUBJECTS.size(); ++j) {
            f << ',';
            if (j < s.grades.size()) f << util::fmt(s.grades[j]);
        }
        f << "\r\n";
    }

    if (!f) { err = "Failed while writing \"" + path + "\"."; return false; }
    return true;
}

}