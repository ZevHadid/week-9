// app_core.cpp
#include "app_core.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

namespace core {

Session session;

namespace {

std::string toLower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool isGender(const std::string& s) { return s == "Male" || s == "Female"; }

bool validateName(const std::string& n, std::string& err) {
    if (trim(n).empty()) { err = "Name cannot be empty."; return false; }
    if (std::any_of(n.begin(), n.end(),
                    [](unsigned char c) { return std::isdigit(c); })) {
        err = "Numbers are not allowed in names.";
        return false;
    }
    return true;
}

bool validateScore(double s, std::string& err) {
    if (!std::isfinite(s) || s < 0.0 || s > 100.0) {
        err = "Score must be between 0 and 100.";
        return false;
    }
    return true;
}

} // namespace

bool formatFromPath(const std::string& path, FileFormat& out) {
    std::string ext = toLower(fs::path(path).extension().string());
    if (ext == ".csv")  { out = FileFormat::Csv;  return true; }
    if (ext == ".xlsx") { out = FileFormat::Xlsx; return true; }
    return false;
}

bool parseScore(const std::string& raw, double& out, std::string& err) {
    try {
        size_t pos = 0;
        std::string s = trim(raw);
        out = std::stod(s, &pos);
        if (pos != s.size()) throw std::invalid_argument("trailing");
    } catch (...) {
        err = "Score must be a valid number.";
        return false;
    }
    return validateScore(out, err);
}

bool validateFile(const std::string& path, std::string& err) {
    FileFormat fmt;
    if (!formatFromPath(path, fmt)) {
        err = "File must end in .csv or .xlsx.";
        return false;
    }
    std::vector<Student> parsed;
    return (fmt == FileFormat::Csv) ? readCsv(path, parsed, err)
                                    : readXlsx(path, parsed, err);
}

std::string normalizeGender(const std::string& raw) {
    std::string s = toLower(trim(raw));
    if (s == "m" || s == "male")   return "Male";
    if (s == "f" || s == "female") return "Female";
    return "";
}

bool openSession(const std::string& path, std::string& err) {
    FileFormat fmt;
    if (!formatFromPath(path, fmt)) {
        err = "File must end in .csv or .xlsx.";
        return false;
    }
    std::vector<Student> parsed;
    bool ok = (fmt == FileFormat::Csv) ? readCsv(path, parsed, err)
                                       : readXlsx(path, parsed, err);
    if (!ok) return false;

    students        = std::move(parsed);
    session.path    = path;
    session.format  = fmt;
    session.bound   = true;
    session.dirty   = false;
    return true;
}

bool newSession(const std::string& path, bool overwrite, std::string& err) {
    FileFormat fmt;
    if (!formatFromPath(path, fmt)) {
        err = "File must end in .csv or .xlsx.";
        return false;
    }
    if (fs::exists(path) && !overwrite) return openSession(path, err);

    students.clear();
    session.path   = path;
    session.format = fmt;
    session.bound  = true;
    session.dirty  = true;  // needs a save to materialise the file
    return true;
}

bool saveSession(std::string& err) {
    if (!session.bound) { err = "No file bound to session."; return false; }
    fs::path p(session.path);
    if (!p.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
        if (ec) { err = "Cannot create folder: " + ec.message(); return false; }
    }
    bool ok = (session.format == FileFormat::Csv)
                ? writeCsv(session.path, students, err)
                : writeXlsx(session.path, students, err);
    if (ok) session.dirty = false;
    return ok;
}

void closeSession() {
    students.clear();
    session = Session{};
}

bool addStudent(const std::string& name, const std::string& gender,
                double score, std::string& err) {
    if (!validateName(name, err))   return false;
    std::string g = normalizeGender(gender);
    if (g.empty()) { err = "Gender must be m or f."; return false; }
    if (!validateScore(score, err)) return false;
    students.push_back({trim(name), score, g});
    session.dirty = true;
    return true;
}

bool modifyStudent(size_t index, const std::string& name,
                   const std::string& gender, double score, std::string& err) {
    if (index >= students.size())   { err = "No student at that index."; return false; }
    if (!validateName(name, err))   return false;
    std::string g = normalizeGender(gender);
    if (g.empty()) { err = "Gender must be m or f."; return false; }
    if (!validateScore(score, err)) return false;
    students[index] = {trim(name), score, g};
    session.dirty = true;
    return true;
}

bool removeStudent(size_t index, std::string& err) {
    if (index >= students.size()) { err = "No student at that index."; return false; }
    students.erase(students.begin() + (long)index);
    session.dirty = true;
    return true;
}

} // namespace core