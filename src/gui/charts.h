#pragma once
#include "core/student.h"

#include <ftxui/dom/elements.hpp>

#include <string>
#include <vector>

namespace gui {

struct Stats {
    size_t count = 0;
    double mean = 0.0, std_dev = 0.0;
    double min = 0.0, max = 0.0, median = 0.0;
};

Stats computeStats(std::vector<double> values);

struct SubjectStats { std::string subject; Stats stats; };
std::vector<SubjectStats> computeSubjectStats(const std::vector<Student>& students);

ftxui::Element summaryStatsView        (const std::vector<Student>& students);
ftxui::Element subjectMeansChart       (const std::vector<Student>& students);
ftxui::Element averageDistributionChart(const std::vector<Student>& students);

}