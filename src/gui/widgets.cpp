#include "widgets.h"
#include "util/str.h"

using namespace ftxui;

namespace gui {

namespace {
const Color MALE_COLOR   = Color::BlueLight;
const Color FEMALE_COLOR = Color::Magenta;
}

Color genderColor(const std::string& gender) {
    return gender == "Female" ? FEMALE_COLOR : MALE_COLOR;
}

std::string averageString(const Student& s) {
    if (s.grades.empty()) return "-";
    return util::fmt(studentAverage(s));
}

Element studentTable(const std::vector<Student>& list) {
    if (list.empty()) return text("No students to display.") | dim;

    Elements header = {
        text("#")          | size(WIDTH, EQUAL, 5),
        text("Grade Year") | size(WIDTH, EQUAL, 14),
        text("Birth Date") | size(WIDTH, EQUAL, 14),
        text("Full Name")  | size(WIDTH, EQUAL, 24),
        text("Gender")     | size(WIDTH, EQUAL, 10),
    };
    for (const auto& sub : SUBJECTS)
        header.push_back(text(sub) | size(WIDTH, EQUAL, 12));
    header.push_back(text("Avg") | size(WIDTH, EQUAL, 8));

    Elements rows;
    rows.push_back(hbox(std::move(header)) | bold);
    rows.push_back(separator());

    for (size_t i = 0; i < list.size(); ++i) {
        const auto& s = list[i];
        Elements row = {
            text(std::to_string(i + 1)) | size(WIDTH, EQUAL, 5),
            text(s.grade_year)          | size(WIDTH, EQUAL, 14),
            text(s.birth_date)          | size(WIDTH, EQUAL, 14),
            text(s.full_name)           | size(WIDTH, EQUAL, 24),
            text(s.gender) | color(genderColor(s.gender)) | size(WIDTH, EQUAL, 10),
        };
        for (size_t j = 0; j < SUBJECTS.size(); ++j) {
            std::string v = (j < s.grades.size()) ? util::fmt(s.grades[j]) : "-";
            row.push_back(text(v) | size(WIDTH, EQUAL, 12));
        }
        row.push_back(text(averageString(s)) | size(WIDTH, EQUAL, 8));
        rows.push_back(hbox(std::move(row)));
    }
    return vbox(std::move(rows));
}

Element borderedPanel(const std::string& title, Element body, const std::string& hint) {
    return vbox({
        text(title) | bold | hcenter,
        separator(),
        body | flex,
        separator(),
        text(hint) | dim,
    }) | border;
}

}