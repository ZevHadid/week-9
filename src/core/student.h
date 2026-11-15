#pragma once
#include <string>
#include <vector>

extern const std::vector<std::string> SUBJECTS;

struct Student {
    std::string grade_year;
    std::string birth_date;
    std::string full_name;
    std::string gender;
    std::vector<double> grades;
};

double studentAverage(const Student& s);
std::vector<std::string> studentColumnHeaders();