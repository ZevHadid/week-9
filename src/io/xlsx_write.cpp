#include "xlsx_write.h"
#include "xml.h"
#include "zip.h"

#include <fstream>

namespace io {

bool writeXlsx(const std::string& path, const std::vector<Student>& list, std::string& err) {
    const std::string hdr = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

    auto headers = studentColumnHeaders();
    int  gradeCol = 4;

    std::string sheet = hdr +
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<cols><col min=\"1\" max=\"1\" width=\"14\" customWidth=\"1\"/>"
        "<col min=\"2\" max=\"2\" width=\"14\" customWidth=\"1\"/>"
        "<col min=\"3\" max=\"3\" width=\"26\" customWidth=\"1\"/>"
        "<col min=\"4\" max=\"" + std::to_string(headers.size()) +
        "\" width=\"12\" customWidth=\"1\"/></cols><sheetData>";

    sheet += "<row r=\"1\">";
    for (size_t i = 0; i < headers.size(); ++i)
        sheet += xml::textCell(xml::columnName(static_cast<int>(i)) + "1", headers[i]);
    sheet += "</row>";

    for (size_t i = 0; i < list.size(); ++i) {
        const Student& s = list[i];
        std::string r = std::to_string(i + 2);
        sheet += "<row r=\"" + r + "\">";
        sheet += xml::textCell("A" + r, s.grade_year);
        sheet += xml::textCell("B" + r, s.birth_date);
        sheet += xml::textCell("C" + r, s.full_name);
        sheet += xml::textCell("D" + r, s.gender);
        for (size_t j = 0; j < SUBJECTS.size() && j < s.grades.size(); ++j)
            sheet += xml::numCell(xml::columnName(gradeCol + static_cast<int>(j)) + r,
                                  s.grades[j]);
        sheet += "</row>";
    }
    sheet += "</sheetData></worksheet>";

    std::vector<zip::Entry> files = {
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
    std::string zip = zip::build(files);
    f.write(zip.data(), static_cast<std::streamsize>(zip.size()));
    if (!f) {
        err = "Failed while writing \"" + path + "\".";
        return false;
    }
    return true;
}

}