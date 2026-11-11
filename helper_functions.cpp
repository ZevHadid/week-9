//helper_functions.cpp
#include "helper_functions.h"
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <fstream>
#include <iomanip>
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