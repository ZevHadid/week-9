#include "student.h"

const std::vector<std::string> SUBJECTS = {"Math", "Science", "English", "History"};

double studentAverage(const Student& s) {
    if (s.grades.empty()) return 0.0;
    double sum = 0.0;
    for (double g : s.grades) sum += g;
    return sum / static_cast<double>(s.grades.size());
}

std::vector<std::string> studentColumnHeaders() {
    std::vector<std::string> h = {"Grade Year", "Birth Date", "Full Name", "Gender"};
    for (const auto& sub : SUBJECTS) h.push_back(sub);
    return h;
}