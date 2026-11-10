//  ____________________________________________________________________________
// |                        Student Records Manager                             |
// |                          Elijah & Zev - 2026                               |
// |                                                                            |
// |  Credits & Third-Party Libraries                                           |
// |                                                                            |
// |  This project uses the following open-source libraries:                    |
// |                                                                            |
// |  [FTXUI] by Arthur Sonzogni                                                |
// |  Functional Terminal User Interface library for C++                        |
// |  (Licensed under the [MIT License (https://github.com/blob/main/LICENSE)). |
// |                                                                            |
// |____________________________________________________________________________|

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "helper_functions.h"  // Student, students, findStudentNByName, sortedStudentsByScore

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

using namespace ftxui;

// ---------------------------------------------------------------- data ----

struct Stats {
    size_t count = 0;
    double mean = 0.0;
    double std_dev = 0.0;
    double min = 0.0;
    double max = 0.0;
    double median = 0.0;
};

// Computed on demand from a copy, so the stored student order is never
// scrambled (the old nth_element call reordered `scores` in place).
Stats computeStats(const std::vector<Student>& students) {
    Stats st;
    if (students.empty()) return st;

    std::vector<double> v;
    v.reserve(students.size());
    for (const auto& s : students) v.push_back(s.score);

    st.count = v.size();
    st.mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();

    if (v.size() > 1) {
        double var_sum = 0.0;
        for (double x : v) var_sum += (x - st.mean) * (x - st.mean);
        st.std_dev = std::sqrt(var_sum / (v.size() - 1));  // sample std dev
    }

    std::sort(v.begin(), v.end());
    st.min = v.front();
    st.max = v.back();
    size_t n = v.size();
    st.median = (n % 2 == 1) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
    return st;
}

// ------------------------------------------------------------- helpers ----

std::string fmt(double value, int precision = 2) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(precision) << value;
    return os.str();
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

Element studentTable(const std::vector<Student>& list) {
    if (list.empty()) return text("No students to display.") | dim;

    Elements rows;
    rows.push_back(hbox({
                       text("#") | size(WIDTH, EQUAL, 5),
                       text("Name") | size(WIDTH, EQUAL, 22),
                       text("Score") | size(WIDTH, EQUAL, 10),
                   }) |
                   bold);
    rows.push_back(separator());
    for (size_t i = 0; i < list.size(); ++i) {
        rows.push_back(hbox({
            text(std::to_string(i + 1)) | size(WIDTH, EQUAL, 5),
            text(list[i].name) | size(WIDTH, EQUAL, 22),
            text(fmt(list[i].score)) | size(WIDTH, EQUAL, 10),
        }));
    }
    return vbox(std::move(rows));
}

// Common window chrome used by every page.
Element frame(const std::string& title, Element body, const std::string& hint) {
    return vbox({
               text(title) | bold | hcenter,
               separator(),
               body | flex,
               separator(),
               text(hint) | dim,
           }) |
           border;
}

// ---------------------------------------------------------------- main ----

enum Page { MENU, ADD, REMOVE, VIEW, SEARCH, SORT, STATS };

int main() {
    auto screen = ScreenInteractive::Fullscreen();

    int page = MENU;

    auto goBack = [&] { page = MENU; };
    auto makeBackButton = [&] { return Button("Back", goBack, ButtonOption::Animated()); };

    // ------------------------------------------------ Add Student page ----
    std::string name_input, score_input, add_status;
    bool add_ok = true;
    Component name_in;

    auto submitStudent = [&] {
        std::string name = trim(name_input);
        if (name.empty()) {
            add_status = "Name cannot be empty.";
            add_ok = false;
            return;
        }

        double score = 0.0;
        try {
            size_t pos = 0;
            std::string raw = trim(score_input);
            score = std::stod(raw, &pos);
            if (pos != raw.size() || !std::isfinite(score)) throw std::invalid_argument("bad");
        } catch (...) {
            add_status = "Score must be a valid number.";
            add_ok = false;
            return;
        }

        students.push_back({name, score});
        add_status = "Added " + name + " with score " + fmt(score) + ".";
        add_ok = true;
        name_input.clear();
        score_input.clear();
        name_in->TakeFocus();  // ready for the next entry
    };

    InputOption score_opt;
    score_opt.on_enter = submitStudent;

    name_in = Input(&name_input, "e.g. Alice");
    auto score_in = Input(&score_input, "e.g. 87.5", score_opt);
    auto add_btn = Button("Add", submitStudent, ButtonOption::Animated());
    auto add_back = makeBackButton();

    auto add_form = Container::Vertical({
        name_in,
        score_in,
        Container::Horizontal({add_btn, add_back}),
    });

    auto add_page = Renderer(add_form, [&] {
        return frame("Add Student",
                     vbox({
                         hbox(text("Name:  "), name_in->Render() | flex),
                         hbox(text("Score: "), score_in->Render() | flex),
                         text(""),
                         hbox(add_btn->Render(), text(" "), add_back->Render()),
                         text(""),
                         text(add_status) | color(add_ok ? Color::Green : Color::Red),
                     }),
                     "Tab/arrows: move | Enter: submit | Esc: back to menu");
    });

    // ---------------------------------------------- Remove Student page ----
    std::vector<std::string> remove_entries;
    int remove_selected = 0;
    std::string remove_status;

    auto refreshRemoveList = [&] {
        remove_entries.clear();
        for (const auto& s : students) remove_entries.push_back(s.name + "  (" + fmt(s.score) + ")");
        if (remove_selected >= static_cast<int>(remove_entries.size()))
            remove_selected = std::max(0, static_cast<int>(remove_entries.size()) - 1);
    };

    MenuOption remove_opt = MenuOption::Vertical();
    remove_opt.on_enter = [&] {
        if (students.empty()) return;
        remove_status = "Removed " + students[remove_selected].name + ".";
        students.erase(students.begin() + remove_selected);
        refreshRemoveList();
    };
    auto remove_menu = Menu(&remove_entries, &remove_selected, remove_opt);
    auto remove_back = makeBackButton();
    auto remove_container = Container::Vertical({remove_menu, remove_back});

    auto remove_page = Renderer(remove_container, [&] {
        Element body = students.empty() ? text("No students to remove.") | dim
                                        : remove_menu->Render() | vscroll_indicator | yframe;
        return frame("Remove Student",
                     vbox({body | flex, text(""), text(remove_status) | color(Color::Yellow),
                           remove_back->Render()}),
                     "Up/Down: select | Enter: remove | Esc: back to menu");
    });

    // ------------------------------------------------- View All page ------
    auto view_back = makeBackButton();
    auto view_page = Renderer(view_back, [&] {
        return frame("All Students",
                     vbox({studentTable(students) | vscroll_indicator | yframe | flex, text(""),
                           view_back->Render()}),
                     "Esc: back to menu");
    });

    // ----------------------------------------------- Search by Name -------
    std::string query;
    auto query_in = Input(&query, "type part of a name...");
    auto search_back = makeBackButton();
    auto search_container = Container::Vertical({query_in, search_back});

    auto search_page = Renderer(search_container, [&] {
        std::string q = trim(query);
        Element results;

        if (q.empty()) {
            results = text("Start typing a name to search.") | dim;
        } else {
            Student found = findStudentNByName(q);  // exact match via helper
            if (!found.name.empty()) {  // sentinel is {"", -1.0}
                results = vbox({text("Exact match:") | bold, studentTable({found})});
            } else {
                // No exact match: offer case-insensitive partial matches instead.
                std::vector<Student> similar;
                std::string lq = toLower(q);
                for (const auto& s : students)
                    if (toLower(s.name).find(lq) != std::string::npos) similar.push_back(s);

                if (similar.empty())
                    results = text("No student named \"" + q + "\".") | dim;
                else
                    results = vbox({text("No exact match. Similar names:") | dim, studentTable(similar)});
            }
        }

        return frame("Search by Name",
                     vbox({hbox(text("Search: "), query_in->Render() | flex), separator(),
                           results | vscroll_indicator | yframe | flex, text(""),
                           search_back->Render()}),
                     "Results update as you type | Esc: back to menu");
    });

    // ------------------------------------------------- Sort by Score ------
    bool descending = true;
    auto flip_btn = Button("Flip order", [&] { descending = !descending; }, ButtonOption::Animated());
    auto sort_back = makeBackButton();
    auto sort_container = Container::Horizontal({flip_btn, sort_back});

    auto sort_page = Renderer(sort_container, [&] {
        std::vector<int> order = sortedStudentsByScore();  // highest first
        if (!descending) std::reverse(order.begin(), order.end());

        std::vector<Student> sorted;
        sorted.reserve(order.size());
        for (int idx : order) sorted.push_back(students[idx]);
        return frame("Sort by Score",
                     vbox({text(descending ? "Order: highest first" : "Order: lowest first") | bold,
                           separator(), studentTable(sorted) | vscroll_indicator | yframe | flex,
                           text(""), hbox(flip_btn->Render(), text(" "), sort_back->Render())}),
                     "Left/Right: move | Enter: press | Esc: back to menu");
    });

    // ---------------------------------------------------- Statistics ------
    auto stats_back = makeBackButton();
    auto stats_page = Renderer(stats_back, [&] {
        Element body;
        if (students.empty()) {
            body = text("No data yet. Add some students first.") | dim;
        } else {
            Stats st = computeStats(students);
            auto row = [](const std::string& label, const std::string& value) {
                return hbox({text(label) | size(WIDTH, EQUAL, 26), text(value) | bold});
            };
            body = vbox({
                row("Count:", std::to_string(st.count)),
                row("Mean:", fmt(st.mean)),
                row("Standard Deviation:", fmt(st.std_dev)),
                row("Minimum Score:", fmt(st.min)),
                row("Maximum Score:", fmt(st.max)),
                row("50th Percentile (Median):", fmt(st.median)),
            });
        }
        return frame("Statistics", vbox({body | flex, text(""), stats_back->Render()}),
                     "Esc: back to menu");
    });

    // -------------------------------------------------------- Menu --------
    std::vector<std::string> menu_items = {"Add Student", "Remove Student", "View All",
                                           "Search by Name", "Sort by Score", "Statistics", "Exit"};
    int menu_selected = 0;

    MenuOption menu_opt = MenuOption::Vertical();
    menu_opt.on_enter = [&] {
        switch (menu_selected) {
            case 0:
                name_input.clear();
                score_input.clear();
                add_status.clear();
                add_ok = true;
                page = ADD;
                name_in->TakeFocus();
                break;
            case 1:
                remove_status.clear();
                refreshRemoveList();
                page = REMOVE;
                break;
            case 2: page = VIEW; break;
            case 3:
                query.clear();
                page = SEARCH;
                query_in->TakeFocus();
                break;
            case 4: page = SORT; break;
            case 5: page = STATS; break;
            case 6: screen.Exit(); break;
        }
    };
    auto menu = Menu(&menu_items, &menu_selected, menu_opt);

    auto menu_page = Renderer(menu, [&] {
        return frame("Student Records Manager",
                     vbox({menu->Render() | hcenter, text(""),
                           text("Students loaded: " + std::to_string(students.size())) | dim | hcenter}),
                     "Up/Down: navigate | Enter: select");
    });

    // ---------------------------------------------------- Root ----------
    auto pages = Container::Tab({menu_page, add_page, remove_page, view_page, search_page,
                                 sort_page, stats_page},
                                &page);

    auto root = CatchEvent(pages, [&](Event e) {
        if (e == Event::Escape && page != MENU) {
            page = MENU;
            return true;
        }
        return false;
    });

    screen.Loop(root);
    return 0;
}