#include "xlsx_read.h"
#include "xml.h"
#include "zip.h"

#include "core/validators.h"
#include "util/str.h"

#include <cctype>
#include <fstream>
#include <iterator>
#include <map>

namespace io {

namespace {

using Grid = std::map<int, std::map<int, std::string>>;

Grid parseSheetGrid(const std::string& sheet) {
    Grid grid;
    size_t pos = 0;
    while ((pos = sheet.find("<c ", pos)) != std::string::npos) {
        size_t end = sheet.find("</c>", pos);
        if (end == std::string::npos) break;
        size_t after = end + 4;
        std::string cell = sheet.substr(pos, after - pos);

        size_t rp = cell.find("r=\"");
        if (rp == std::string::npos) { pos = after; continue; }
        size_t re = cell.find('"', rp + 3);
        std::string ref = cell.substr(rp + 3, re - rp - 3);

        int col = 0, row = 0;
        size_t i = 0;
        while (i < ref.size() && std::isalpha(static_cast<unsigned char>(ref[i]))) {
            col = col * 26 + (std::toupper(static_cast<unsigned char>(ref[i])) - 'A' + 1);
            ++i;
        }
        --col;
        while (i < ref.size() && std::isdigit(static_cast<unsigned char>(ref[i]))) {
            row = row * 10 + (ref[i] - '0');
            ++i;
        }

        bool isStr = cell.find("t=\"inlineStr\"") != std::string::npos ||
                     cell.find("t=\"str\"")       != std::string::npos;
        std::string val;
        if (isStr) {
            size_t ts = cell.find("<t>");
            if (ts != std::string::npos) {
                size_t te = cell.find("</t>", ts);
                if (te != std::string::npos)
                    val = xml::unescape(cell.substr(ts + 3, te - ts - 3));
            }
        } else {
            size_t vs = cell.find("<v>");
            if (vs != std::string::npos) {
                size_t ve = cell.find("</v>", vs);
                if (ve != std::string::npos) val = cell.substr(vs + 3, ve - vs - 3);
            }
        }
        if (!val.empty()) grid[row][col] = val;
        pos = after;
    }
    return grid;
}

}

bool readXlsx(const std::string& path, std::vector<Student>& out,
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

    std::string sheet;
    if (!zip::extract(content, "xl/worksheets/sheet1.xml", sheet)) {
        err = "Not a valid Student Records file (expected an .xlsx exported by this app).";
        return false;
    }

    auto grid = parseSheetGrid(sheet);
    auto r1 = grid.find(1);
    if (r1 == grid.end()) { err = "File has no header row."; return false; }

    auto headerAt = [&](int col) -> std::string {
        auto it = r1->second.find(col);
        return it == r1->second.end() ? std::string() : util::toLower(util::trim(it->second));
    };

    auto expected = studentColumnHeaders();
    for (size_t i = 0; i < expected.size(); ++i) {
        if (headerAt(static_cast<int>(i)) != util::toLower(expected[i])) {
            err = "Unexpected header at column " + std::to_string(i + 1) +
                  " (expected \"" + expected[i] + "\").";
            return false;
        }
    }

    size_t skipped = 0;
    int    gradeCol = 4;
    for (auto it = grid.begin(); it != grid.end(); ++it) {
        if (it->first <= 1) continue;
        const auto& cols = it->second;

        auto cellAt = [&](int col) -> std::string {
            auto cit = cols.find(col);
            return cit == cols.end() ? std::string() : cit->second;
        };

        Student s;
        s.grade_year = util::trim(cellAt(0));
        s.birth_date = util::trim(cellAt(1));
        s.full_name  = util::trim(cellAt(2));
        s.gender     = core::normalizeGender(cellAt(3));

        if (s.full_name.empty()) { ++skipped; continue; }

        s.grades.assign(SUBJECTS.size(), 0.0);
        bool rowOk = true;
        for (size_t j = 0; j < SUBJECTS.size(); ++j) {
            double g = 0.0;
            if (!util::parseDouble(cellAt(gradeCol + static_cast<int>(j)), g)) {
                rowOk = false;
                break;
            }
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