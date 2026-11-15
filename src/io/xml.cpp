#include "xml.h"
#include "util/str.h"

namespace io::xml {

std::string escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            default:  out += c;
        }
    }
    return out;
}

std::string unescape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') { out += s[i]; continue; }
        if      (s.compare(i, 5, "&amp;")  == 0) { out += '&';  i += 4; }
        else if (s.compare(i, 4, "&lt;")   == 0) { out += '<';  i += 3; }
        else if (s.compare(i, 4, "&gt;")   == 0) { out += '>';  i += 3; }
        else if (s.compare(i, 6, "&quot;") == 0) { out += '"';  i += 5; }
        else if (s.compare(i, 6, "&apos;") == 0) { out += '\''; i += 5; }
        else                                     { out += s[i]; }
    }
    return out;
}

std::string textCell(const std::string& ref, const std::string& s) {
    return "<c r=\"" + ref + "\" t=\"inlineStr\"><is><t>" + escape(s) + "</t></is></c>";
}

std::string numCell(const std::string& ref, double v) {
    return "<c r=\"" + ref + "\"><v>" + util::fmt(v) + "</v></c>";
}

std::string columnName(int col) {
    std::string out;
    col++;
    while (col > 0) {
        col--;
        out = static_cast<char>('A' + col % 26) + out;
        col /= 26;
    }
    return out;
}

}