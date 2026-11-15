#include "csv_read.h"
#include "core/validators.h"
#include "util/str.h"

#include <fstream>
#include <iterator>

namespace io {

namespace {

std::vector<std::vector<std::string>> splitCsv(const std::string& content) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool inQuotes = false;
    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < content.size() && content[i + 1] == '"') { field += '"'; ++i; }
                else inQuotes = false;
            } else field += c;
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            row.push_back(field); field.clear();
        } else if (c == '\r') {
        } else if (c == '\n') {
            row.push_back(field); field.clear();
            rows.push_back(row); row.clear();
        } else {
            field += c;
        }
    }
    if (!field.empty() || !row.empty()) {
        row.push_back(field);
        rows.push_back(row);
    }
    return rows;
}

}

bool readCsv(const std::string& path, std::vector<Student>& out,
             std::string& err, bool& wasEmpty) {
    wasEmpty = false;
    out.clear();

    std::ifstream f(path, std::ios::binary);
    if (!f) { err = "Could not open \"" + path + "\"."; return false; }

    f.seekg(0, std::ios::end);
    std::streamoff size = f.tellg();
    if (size <= 0) { wasEmpty = true; return true; }
    f.seekg(0, std::ios::beg);

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF)
        content.erase(0, 3);

    auto rows = splitCsv(content);
    if (rows.empty()) { err = "File is empty."; return false; }

    auto expected = studentColumnHeaders();
    if (rows[0].size() < expected.size()) {
        err = "File has fewer columns than expected.";
        return false;
    }
    for (size_t i = 0; i < expected.size(); ++i) {
        if (util::toLower(util::trim(rows[0][i])) != util::toLower(expected[i])) {
            err = "Unexpected header at column " + std::to_string(i + 1) +
                  " (expected \"" + expected[i] + "\").";
            return false;
        }
    }

    size_t skipped = 0;
    for (size_t r = 1; r < rows.size(); ++r) {
        const auto& rw = rows[r];
        if (rw.size() < expected.size()) { ++skipped; continue; }

        Student s;
        s.grade_year = util::trim(rw[0]);
        s.birth_date = util::trim(rw[1]);
        s.full_name  = util::trim(rw[2]);
        s.gender     = core::normalizeGender(rw[3]);

        if (s.full_name.empty()) { ++skipped; continue; }

        s.grades.assign(SUBJECTS.size(), 0.0);
        bool rowOk = true;
        for (size_t j = 0; j < SUBJECTS.size(); ++j) {
            double g = 0.0;
            if (!util::parseDouble(rw[4 + j], g)) { rowOk = false; break; }
            s.grades[j] = g;
        }
        if (!rowOk) { ++skipped; continue; }
        out.push_back(std::move(s));
    }

    if (out.empty() && skipped == 0) {
        err = "File has no data rows.";
        return false;
    }
    return true;
}

}