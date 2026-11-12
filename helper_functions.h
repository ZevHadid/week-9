//helper_functions.h
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

// Function declarations
Student findStudentNByName(std::string name);
std::vector<int> sortedStudentsByScore();

// Export: both return false and fill `err` on failure.
bool writeCsv(const std::string& path, const std::vector<Student>& list, std::string& err);
bool writeXlsx(const std::string& path, const std::vector<Student>& list, std::string& err);

// Import: return false and fill `err` on failure. `out` is only modified on success.
bool readCsv(const std::string& path, std::vector<Student>& out, std::string& err);
bool readXlsx(const std::string& path, std::vector<Student>& out, std::string& err);