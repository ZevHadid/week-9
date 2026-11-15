#include "str.h"

#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace util {

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string fmt(double value, int precision) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(precision) << value;
    return os.str();
}

bool parseDouble(const std::string& raw, double& out) {
    try {
        size_t pos = 0;
        std::string s = trim(raw);
        out = std::stod(s, &pos);
        if (pos != s.size()) return false;
        return std::isfinite(out);
    } catch (...) {
        return false;
    }
}

}