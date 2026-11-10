#include "helper_functions.h"  // <-- FIXED: Matches your actual filename
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>     

// Allocating memory for globals (No struct re-definitions here)
std::vector<Student> students;
std::vector<double> scores;
int count = 0;
double mean = 0.0;
double variance_sum = 0.0;
double std_dev = 0.0;
double min_score = 0.0;
double max_score = 0.0;
double p50 = 0.0;

Student findStudentNByName(std::string name) {
    for (const auto& student : students) {
        if (student.name == name) {
            return student;
        }
    }
    return {"", -1.0};
}

std::vector<int> sortedStudentsByScore() {
    std::vector<int> indices(students.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return students[a].score > students[b].score;
    });

    return indices;
}
