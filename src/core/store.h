#pragma once
#include "student.h"

#include <cstddef>
#include <string>
#include <vector>

namespace core {

class Store {
public:
    const std::vector<Student>& all() const { return students_; }
    size_t size() const { return students_.size(); }
    const Student& at(size_t i) const { return students_[i]; }

    bool bound() const { return bound_; }
    bool dirty() const { return dirty_; }
    const std::string& path() const { return path_; }

    bool add   (const Student& s, std::string& err);
    bool modify(size_t index, const Student& s, std::string& err);
    bool remove(size_t index, std::string& err);

    const Student* findByName(const std::string& full_name) const;
    std::vector<size_t> indicesSortedByAverageDesc() const;

    bool open    (const std::string& path, std::string& err);
    bool exportTo(const std::string& path, std::string& err);
    void clear();

private:
    std::vector<Student> students_;
    std::string          path_;
    bool                 bound_ = false;
    bool                 dirty_ = false;

    bool contains(const std::string& name, const std::string& gender) const;
    static Student canonical(const Student& s);
};

}