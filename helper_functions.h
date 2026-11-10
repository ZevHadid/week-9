#pragma once

#include <string>
#include <vector>

// Defined ONLY once here
struct Student {
    std::string name;
    double score;
    std::string gender = "Male";   // "Male" or "Female"
};

// Global variables made available to other files
extern std::vector<Student> students;
extern std::vector<double> scores;
extern int count;
extern double mean;
extern double variance_sum;
extern double std_dev;
extern double min_score;
extern double max_score;
extern double p50;

// Function declarations (FIXED line 28 here)
Student findStudentNByName(std::string name);
std::vector<int> sortedStudentsByScore();
