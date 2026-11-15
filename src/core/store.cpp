#include "store.h"
#include "validators.h"

#include "io/read.h"
#include "io/write.h"
#include "util/path.h"
#include "util/str.h"

#include <algorithm>
#include <filesystem>
#include <numeric>

namespace fs = std::filesystem;

namespace core {

Student Store::canonical(const Student& s) {
    Student c = s;
    c.grade_year = util::trim(c.grade_year);
    c.birth_date = util::trim(c.birth_date);
    c.full_name  = util::trim(c.full_name);
    c.gender     = normalizeGender(c.gender);
    return c;
}

bool Store::contains(const std::string& name, const std::string& gender) const {
    for (const auto& s : students_)
        if (s.full_name == name && s.gender == gender) return true;
    return false;
}

bool Store::add(const Student& s, std::string& err) {
    if (!validateStudent(s, err)) return false;
    Student c = canonical(s);
    if (contains(c.full_name, c.gender)) {
        err = "A student with that name and gender already exists.";
        return false;
    }
    students_.push_back(std::move(c));
    dirty_ = true;
    return true;
}

bool Store::modify(size_t index, const Student& s, std::string& err) {
    if (index >= students_.size()) { err = "No student at that index."; return false; }
    if (!validateStudent(s, err)) return false;
    Student c = canonical(s);
    bool same = students_[index].full_name == c.full_name &&
                students_[index].gender    == c.gender;
    if (!same && contains(c.full_name, c.gender)) {
        err = "A student with that name and gender already exists.";
        return false;
    }
    students_[index] = std::move(c);
    dirty_ = true;
    return true;
}

bool Store::remove(size_t index, std::string& err) {
    if (index >= students_.size()) { err = "No student at that index."; return false; }
    students_.erase(students_.begin() + static_cast<std::ptrdiff_t>(index));
    dirty_ = true;
    return true;
}

const Student* Store::findByName(const std::string& full_name) const {
    for (const auto& s : students_)
        if (s.full_name == full_name) return &s;
    return nullptr;
}

std::vector<size_t> Store::indicesSortedByAverageDesc() const {
    std::vector<size_t> idx(students_.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
        return studentAverage(students_[a]) > studentAverage(students_[b]);
    });
    return idx;
}

bool Store::open(const std::string& rawPath, std::string& err) {
    std::string resolved = util::resolvePath(rawPath);
    if (resolved.empty()) { err = "File path is empty."; return false; }

    fs::path p(resolved);
    std::string ext = util::toLower(p.extension().string());
    if (ext != ".csv" && ext != ".xlsx") {
        err = "File must end in .csv or .xlsx.";
        return false;
    }

    std::vector<Student> loaded;
    if (fs::exists(resolved)) {
        bool empty = false;
        if (!io::readFile(resolved, loaded, err, empty)) return false;
    }

    students_ = std::move(loaded);
    path_     = resolved;
    bound_    = true;
    dirty_    = false;
    return true;
}

bool Store::exportTo(const std::string& rawPath, std::string& err) {
    std::string resolved = util::resolvePath(rawPath);
    if (resolved.empty()) { err = "File path is empty."; return false; }

    fs::path p(resolved);
    std::string ext = util::toLower(p.extension().string());
    if (ext != ".csv" && ext != ".xlsx") {
        err = "File must end in .csv or .xlsx.";
        return false;
    }
    if (!p.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
        if (ec) { err = "Cannot create folder: " + ec.message(); return false; }
    }
    if (!io::writeFile(resolved, students_, err)) return false;

    path_  = resolved;
    bound_ = true;
    dirty_ = false;
    return true;
}

void Store::clear() {
    students_.clear();
    path_.clear();
    bound_ = false;
    dirty_ = false;
}

}