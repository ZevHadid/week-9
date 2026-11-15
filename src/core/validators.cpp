#include "validators.h"
#include "util/str.h"

#include <cctype>

namespace core {

std::string normalizeGender(const std::string& raw) {
    std::string s = util::toLower(util::trim(raw));
    if (s == "m" || s == "male")   return "Male";
    if (s == "f" || s == "female") return "Female";
    return "";
}

bool validateGradeYear(const std::string& raw, std::string& err) {
    std::string t = util::trim(raw);
    if (t.empty())     { err = "Grade year cannot be empty."; return false; }
    if (t.size() > 32) { err = "Grade year is too long.";     return false; }
    return true;
}

bool validateBirthDate(const std::string& raw, std::string& err) {
    std::string t = util::trim(raw);
    if (t.size() != 10) { err = "Birth date must be YYYY-MM-DD."; return false; }
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9})
        if (!std::isdigit(static_cast<unsigned char>(t[i]))) {
            err = "Birth date must be YYYY-MM-DD.";
            return false;
        }
    if (t[4] != '-' || t[7] != '-') {
        err = "Birth date must be YYYY-MM-DD.";
        return false;
    }
    int month = std::stoi(t.substr(5, 2));
    int day   = std::stoi(t.substr(8, 2));
    if (month < 1 || month > 12) { err = "Birth date has an invalid month."; return false; }
    if (day   < 1 || day   > 31) { err = "Birth date has an invalid day.";   return false; }
    return true;
}

bool validateFullName(const std::string& raw, std::string& err) {
    std::string t = util::trim(raw);
    if (t.empty())     { err = "Full name cannot be empty."; return false; }
    if (t.size() > 80) { err = "Full name is too long.";     return false; }
    if (t.find('\n') != std::string::npos ||
        t.find('\r') != std::string::npos) {
        err = "Full name cannot contain line breaks.";
        return false;
    }
    return true;
}

bool validateGrade(double value, std::string& err) {
    if (value < 0.0 || value > 100.0) {
        err = "Grade must be between 0 and 100.";
        return false;
    }
    return true;
}

bool parseGrade(const std::string& raw, double& out, std::string& err) {
    if (!util::parseDouble(raw, out)) {
        err = "Grade must be a valid number.";
        return false;
    }
    return validateGrade(out, err);
}

bool validateStudent(const Student& s, std::string& err) {
    if (!validateGradeYear(s.grade_year, err)) return false;
    if (!validateBirthDate(s.birth_date, err)) return false;
    if (!validateFullName(s.full_name, err))   return false;
    if (normalizeGender(s.gender).empty()) {
        err = "Gender must be m or f.";
        return false;
    }
    if (s.grades.size() != SUBJECTS.size()) {
        err = "Expected " + std::to_string(SUBJECTS.size()) + " grade(s).";
        return false;
    }
    for (double g : s.grades)
        if (!validateGrade(g, err)) return false;
    return true;
}

}