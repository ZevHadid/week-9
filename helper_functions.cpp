//helper_functions.cpp
#include "helper_functions.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>

// Allocating memory for globals (No struct re-definitions here)
std::vector<Student> students;
std::vector<double> scores;
int count = 0;
double mean = 0.0;
double variance_sum = 0.0;
double std_dev = 0.0;
double min_score = 0.0;
double max_score = 0.0;
double p50 = 0.0;

Student findStudentNByName(std::string name) {
    for (const auto& student : students) {
        if (student.name == name) {
            return student;
        }
    }
    return {"", -1.0};
}

std::vector<int> sortedStudentsByScore() {
    std::vector<int> indices(students.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return students[a].score > students[b].score;
    });

    return indices;
}

// ======================================================================
// Export (CSV + minimal .xlsx). Internals are private to this file.
// ======================================================================

namespace {

std::string num(double v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << v;
    return os.str();
}

// ---- CSV helpers ------------------------------------------------------

std::string csvField(std::string s) {
    // Stop spreadsheet apps from treating a name as a formula.
    if (!s.empty() && (s[0] == '=' || s[0] == '+' || s[0] == '-' || s[0] == '@')) s = "'" + s;
    bool quote = s.find_first_of(",\"\r\n") != std::string::npos;
    if (!quote) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    return out + "\"";
}

// ---- XLSX helpers (zip container, stored/uncompressed entries) --------

std::string xmlEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

uint32_t crc32Of(const std::string& data) {
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char c : data) {
        crc ^= c;
        for (int i = 0; i < 8; ++i) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

void put16(std::string& o, uint16_t v) {
    o += static_cast<char>(v & 0xFF);
    o += static_cast<char>((v >> 8) & 0xFF);
}

void put32(std::string& o, uint32_t v) {
    for (int i = 0; i < 4; ++i) o += static_cast<char>((v >> (8 * i)) & 0xFF);
}

struct ZipEntry {
    std::string name, data;
};

std::string buildZip(const std::vector<ZipEntry>& entries) {
    const uint16_t DOS_DATE = 0x0021;  // 1980-01-01
    std::string out, central;

    for (const auto& e : entries) {
        uint32_t offset = static_cast<uint32_t>(out.size());
        uint32_t crc = crc32Of(e.data);
        uint32_t size = static_cast<uint32_t>(e.data.size());

        put32(out, 0x04034b50);  // local file header
        put16(out, 20);
        put16(out, 0);  // flags
        put16(out, 0);  // method: stored
        put16(out, 0);  // time
        put16(out, DOS_DATE);
        put32(out, crc);
        put32(out, size);
        put32(out, size);
        put16(out, static_cast<uint16_t>(e.name.size()));
        put16(out, 0);
        out += e.name;
        out += e.data;

        put32(central, 0x02014b50);  // central directory header
        put16(central, 20);
        put16(central, 20);
        put16(central, 0);
        put16(central, 0);
        put16(central, 0);
        put16(central, DOS_DATE);
        put32(central, crc);
        put32(central, size);
        put32(central, size);
        put16(central, static_cast<uint16_t>(e.name.size()));
        put16(central, 0);  // extra
        put16(central, 0);  // comment
        put16(central, 0);  // disk
        put16(central, 0);  // internal attrs
        put32(central, 0);  // external attrs
        put32(central, offset);
        central += e.name;
    }

    uint32_t cd_offset = static_cast<uint32_t>(out.size());
    out += central;

    put32(out, 0x06054b50);  // end of central directory
    put16(out, 0);
    put16(out, 0);
    put16(out, static_cast<uint16_t>(entries.size()));
    put16(out, static_cast<uint16_t>(entries.size()));
    put32(out, static_cast<uint32_t>(central.size()));
    put32(out, cd_offset);
    put16(out, 0);
    return out;
}

std::string textCell(const std::string& ref, const std::string& s) {
    return "<c r=\"" + ref + "\" t=\"inlineStr\"><is><t>" + xmlEscape(s) + "</t></is></c>";
}

}  // namespace

bool writeCsv(const std::string& path, const std::vector<Student>& list, std::string& err) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        err = "Could not open \"" + path + "\" for writing (is it open in another program?).";
        return false;
    }
    f << "\xEF\xBB\xBF";  // UTF-8 BOM so Excel reads non-ASCII names correctly
    f << "Name,Gender,Score\r\n";
    for (const auto& s : list)
        f << csvField(s.name) << ',' << csvField(s.gender) << ',' << num(s.score) << "\r\n";
    if (!f) {
        err = "Failed while writing \"" + path + "\".";
        return false;
    }
    return true;
}

bool writeXlsx(const std::string& path, const std::vector<Student>& list, std::string& err) {
    const std::string hdr = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

    std::string sheet = hdr +
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<cols><col min=\"1\" max=\"1\" width=\"28\" customWidth=\"1\"/>"
        "<col min=\"2\" max=\"3\" width=\"12\" customWidth=\"1\"/></cols><sheetData>";
    sheet += "<row r=\"1\">" + textCell("A1", "Name") + textCell("B1", "Gender") +
             textCell("C1", "Score") + "</row>";
    for (size_t i = 0; i < list.size(); ++i) {
        std::string r = std::to_string(i + 2);
        sheet += "<row r=\"" + r + "\">" + textCell("A" + r, list[i].name) +
                 textCell("B" + r, list[i].gender) + "<c r=\"C" + r + "\"><v>" +
                 num(list[i].score) + "</v></c></row>";
    }
    sheet += "</sheetData></worksheet>";

    std::vector<ZipEntry> files = {
        {"[Content_Types].xml",
         hdr + "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
               "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
               "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
               "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
               "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
               "</Types>"},
        {"_rels/.rels",
         hdr + "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
               "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
               "</Relationships>"},
        {"xl/workbook.xml",
         hdr + "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
               "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
               "<sheets><sheet name=\"Students\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>"},
        {"xl/_rels/workbook.xml.rels",
         hdr + "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
               "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
               "</Relationships>"},
        {"xl/worksheets/sheet1.xml", sheet},
    };

    std::ofstream f(path, std::ios::binary);
    if (!f) {
        err = "Could not open \"" + path + "\" for writing (is it open in Excel?).";
        return false;
    }
    std::string zip = buildZip(files);
    f.write(zip.data(), static_cast<std::streamsize>(zip.size()));
    if (!f) {
        err = "Failed while writing \"" + path + "\".";
        return false;
    }
    return true;
}

// ======================================================================
// Import (CSV + .xlsx produced by this app)
// ======================================================================

namespace {

std::string xmlUnescape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') { out += s[i]; continue; }
        if (s.compare(i, 5, "&amp;")  == 0) { out += '&';  i += 4; }
        else if (s.compare(i, 4, "&lt;")  == 0) { out += '<';  i += 3; }
        else if (s.compare(i, 4, "&gt;")  == 0) { out += '>';  i += 3; }
        else if (s.compare(i, 6, "&quot;") == 0) { out += '"';  i += 5; }
        else if (s.compare(i, 6, "&apos;") == 0) { out += '\''; i += 5; }
        else out += s[i];
    }
    return out;
}

// Reads an entry from a STORED (method 0) zip. Our exporter only writes
// stored entries, so we don't need a DEFLATE decoder here.
bool extractZipEntry(const std::string& zipData, const std::string& entryName,
                     std::string& out) {
    size_t pos = 0;
    auto le16 = [&](size_t p) { return (uint16_t)((uint8_t)zipData[p] |
                                    ((uint8_t)zipData[p + 1] << 8)); };
    auto le32 = [&](size_t p) { return (uint32_t)((uint8_t)zipData[p] |
                                    ((uint8_t)zipData[p + 1] << 8) |
                                    ((uint8_t)zipData[p + 2] << 16) |
                                    ((uint8_t)zipData[p + 3] << 24)); };

    while (pos + 30 <= zipData.size()) {
        if (le32(pos) != 0x04034b50u) break;               // local file header
        uint16_t method    = le16(pos + 8);
        uint32_t compSize  = le32(pos + 18);
        uint16_t nameLen   = le16(pos + 26);
        uint16_t extraLen  = le16(pos + 28);
        size_t   nameStart = pos + 30;
        if (nameStart + nameLen + extraLen + compSize > zipData.size()) break;

        if (zipData.compare(nameStart, nameLen, entryName) == 0) {
            if (method != 0) return false;                 // DEFLATE unsupported
            out.assign(zipData, nameStart + nameLen + extraLen, compSize);
            return true;
        }
        pos = nameStart + nameLen + extraLen + compSize;
    }
    return false;
}

// Minimal xlsx reader: understands inlineStr and numeric cells, which is
// exactly what writeXlsx emits. Row 1 is treated as a header.
bool parseSheet(const std::string& sheet, std::vector<Student>& out, std::string& err) {
    struct Cell { std::string val; bool isNum = false; };
    std::map<int, std::map<int, Cell>> grid;

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
        while (i < ref.size() && std::isalpha((unsigned char)ref[i])) {
            col = col * 26 + (std::toupper((unsigned char)ref[i]) - 'A' + 1); ++i;
        }
        --col;
        while (i < ref.size() && std::isdigit((unsigned char)ref[i])) {
            row = row * 10 + (ref[i] - '0'); ++i;
        }

        bool isStr = cell.find("t=\"inlineStr\"") != std::string::npos ||
                     cell.find("t=\"str\"")       != std::string::npos;
        Cell c;
        if (isStr) {
            size_t ts = cell.find("<t>");
            if (ts != std::string::npos) {
                size_t te = cell.find("</t>", ts);
                if (te != std::string::npos)
                    c.val = xmlUnescape(cell.substr(ts + 3, te - ts - 3));
            }
        } else {
            size_t vs = cell.find("<v>");
            if (vs != std::string::npos) {
                size_t ve = cell.find("</v>", vs);
                if (ve != std::string::npos) {
                    c.val   = cell.substr(vs + 3, ve - vs - 3);
                    c.isNum = true;
                }
            }
        }
        if (!c.val.empty()) grid[row][col] = c;
        pos = after;
    }

    std::vector<Student> parsed;
    for (auto& [row, cols] : grid) {
        if (row == 1) continue;                            // header
        auto get = [&](int c) -> std::string {
            auto it = cols.find(c);
            return it == cols.end() ? std::string() : it->second.val;
        };
        std::string name = get(0);
        if (name.empty()) continue;
        std::string gender = get(1);
        if (gender != "Male" && gender != "Female") gender = "Male";
        double score = 0.0;
        try { score = std::stod(get(2)); } catch (...) { continue; }
        parsed.push_back({name, score, gender});
    }
    if (parsed.empty()) { err = "No valid student rows found in worksheet."; return false; }
    out = std::move(parsed);
    return true;
}

}  // namespace

bool readCsv(const std::string& path, std::vector<Student>& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { err = "Could not open \"" + path + "\"."; return false; }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    // Strip UTF-8 BOM (writeCsv adds one).
    if (content.size() >= 3 &&
        (uint8_t)content[0] == 0xEF && (uint8_t)content[1] == 0xBB &&
        (uint8_t)content[2] == 0xBF)
        content.erase(0, 3);

    // Split into rows/fields, honouring RFC-4180 quoting.
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
            // skip; \n handles the row break
        } else if (c == '\n') {
            row.push_back(field); field.clear();
            if (!row.empty()) rows.push_back(row);
            row.clear();
        } else {
            field += c;
        }
    }
    if (!field.empty() || !row.empty()) {
        row.push_back(field);
        rows.push_back(row);
    }
    if (rows.empty()) { err = "File is empty."; return false; }

    // Skip a header row if it exists.
    size_t start = 0;
    if (!rows[0].empty()) {
        std::string first = rows[0][0];
        for (char& c : first) c = (char)std::tolower((unsigned char)c);
        if (first.find("name") != std::string::npos) start = 1;
    }

    std::vector<Student> parsed;
    for (size_t r = start; r < rows.size(); ++r) {
        const auto& rw = rows[r];
        if (rw.size() < 3 || rw[0].empty()) continue;

        std::string name = rw[0];
        // Undo csvField's formula-injection guard.
        if (name.size() >= 2 && name[0] == '\'' &&
            (name[1] == '=' || name[1] == '+' || name[1] == '-' || name[1] == '@'))
            name = name.substr(1);

        std::string gender = rw[1];
        if (gender != "Male" && gender != "Female") gender = "Male";

        double score = 0.0;
        try { score = std::stod(rw[2]); } catch (...) { continue; }
        parsed.push_back({name, score, gender});
    }

    if (parsed.empty()) { err = "No valid student rows found."; return false; }
    out = std::move(parsed);
    return true;
}

bool readXlsx(const std::string& path, std::vector<Student>& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { err = "Could not open \"" + path + "\"."; return false; }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    std::string sheet;
    if (!extractZipEntry(content, "xl/worksheets/sheet1.xml", sheet)) {
        err = "Could not read \"" + path +
              "\". Only .xlsx files produced by this app's Export are supported "
              "(Excel-saved files use compression this build can't decode — "
              "save as CSV instead).";
        return false;
    }
    return parseSheet(sheet, out, err);
}