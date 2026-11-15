#pragma once
#include "core/student.h"

#include <ftxui/dom/elements.hpp>

#include <string>
#include <vector>

namespace gui {

ftxui::Color genderColor(const std::string& gender);
std::string   averageString(const Student& s);

ftxui::Element studentTable(const std::vector<Student>& list);
ftxui::Element borderedPanel(const std::string& title, ftxui::Element body,
                             const std::string& hint);

}