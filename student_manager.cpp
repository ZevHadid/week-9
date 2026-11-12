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
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>

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

// Computed on demand from a copy, so the stored student order is never scrambled.
Stats computeStats(const std::vector<Student>& list) {
    Stats st;
    if (list.empty()) return st;

    std::vector<double> v;
    v.reserve(list.size());
    for (const auto& s : list) v.push_back(s.score);

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

// Students of one gender ("Male" / "Female"), in stored order.
std::vector<Student> byGender(const std::string& g) {
    std::vector<Student> out;
    for (const auto& s : students)
        if (s.gender == g) out.push_back(s);
    return out;
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

bool checkNumber(const std::string& str) {
    return std::any_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

// Shared by the Add and Modify pages. Returns false and fills `err` on bad input.
bool validateEntry(const std::string& raw_name, const std::string& raw_score,
                   std::string& name, double& score, std::string& err) {
    name = trim(raw_name);
    if (name.empty()) {
        err = "Name cannot be empty.";
        return false;
    }

    try {
        size_t pos = 0;
        std::string raw = trim(raw_score);
        score = std::stod(raw, &pos);
        if (pos != raw.size() || !std::isfinite(score) || score > 100.0) {
            throw std::invalid_argument("bad");
        }
    } catch (...) {
        err = "Score must be a valid number less than or equal to 100.";
        return false;
    }

    if (checkNumber(name)) {
        err = "Numbered values aren't allowed in the name field!";
        return false;
    }
    return true;
}

const Color MALE_COLOR = Color::BlueLight;
const Color FEMALE_COLOR = Color::Magenta;

Color genderColor(const std::string& g) { return g == "Female" ? FEMALE_COLOR : MALE_COLOR; }

Element studentTable(const std::vector<Student>& list) {
    if (list.empty()) return text("No students to display.") | dim;

    Elements rows;
    rows.push_back(hbox({
                       text("#") | size(WIDTH, EQUAL, 5),
                       text("Name") | size(WIDTH, EQUAL, 22),
                       text("Gender") | size(WIDTH, EQUAL, 10),
                       text("Score") | size(WIDTH, EQUAL, 10),
                   }) |
                   bold);
    rows.push_back(separator());
    for (size_t i = 0; i < list.size(); ++i) {
        rows.push_back(hbox({
            text(std::to_string(i + 1)) | size(WIDTH, EQUAL, 5),
            text(list[i].name) | size(WIDTH, EQUAL, 22),
            text(list[i].gender) | color(genderColor(list[i].gender)) | size(WIDTH, EQUAL, 10),
            text(fmt(list[i].score)) | size(WIDTH, EQUAL, 10),
        }));
    }
    return vbox(std::move(rows));
}

// ------------------------------------------------------------- charts ----

// One horizontal bar: label | gauge | value
Element barRow(const std::string& label, double frac, const std::string& value, Color c,
               int label_width = 16) {
    frac = std::clamp(frac, 0.0, 1.0);
    return hbox({
        text(label) | size(WIDTH, EQUAL, label_width),
        gauge(static_cast<float>(frac)) | color(c) | flex,
        text(" " + value) | size(WIDTH, EQUAL, 9),
    });
}

// Min ──━━━●◆━━── Max ruler showing mean, median and a +/-1 std dev band.
Element rangeRuler(const std::string& label, const Stats& st, Color c) {
    const int W = 48;
    double span = st.max - st.min;
    auto pos = [&](double v) {
        if (span <= 0.0) return 0;
        int p = static_cast<int>(std::lround((v - st.min) / span * (W - 1)));
        return std::clamp(p, 0, W - 1);
    };

    std::vector<std::string> cells(W, "─");
    int a = pos(std::max(st.min, st.mean - st.std_dev));
    int b = pos(std::min(st.max, st.mean + st.std_dev));
    for (int i = a; i <= b; ++i) cells[i] = "━";
    cells[pos(st.median)] = "◆";
    cells[pos(st.mean)] = (pos(st.mean) == pos(st.median)) ? "◉" : "●";

    std::string line;
    for (const auto& cell : cells) line += cell;

    return hbox(text(label) | size(WIDTH, EQUAL, 9), text(fmt(st.min)), text(" "),
                text(line) | color(c), text(" "), text(fmt(st.max)));
}

// Every student's score as a horizontal bar, highest first, coloured by gender.
Element scoreBarsChart(const Stats& st) {
    std::vector<int> order = sortedStudentsByScore();
    double lo = std::min(0.0, st.min);
    double hi = std::max(st.max, lo + 1e-9);

    Elements rows;
    for (int idx : order) {
        const Student& s = students[idx];
        std::string tag = (s.gender == "Female") ? " (F)" : " (M)";
        rows.push_back(barRow(s.name + tag, (s.score - lo) / (hi - lo), fmt(s.score),
                              genderColor(s.gender), 20));
    }

    return vbox({
        vbox(std::move(rows)) | vscroll_indicator | yframe | flex,
        separator(),
        hbox(text("Male") | color(MALE_COLOR), text("   "), text("Female") | color(FEMALE_COLOR),
             text("   Overall mean: " + fmt(st.mean)) | dim),
    });
}

// Histogram: students per score range, one bar per gender in each range.
Element distributionChart(const Stats& st) {
    size_t n = students.size();
    int bins = (st.max == st.min)
                   ? 1
                   : std::clamp(static_cast<int>(std::ceil(std::sqrt(static_cast<double>(n)))), 3, 10);
    double width = (st.max - st.min) / bins;

    std::vector<int> male(bins, 0), female(bins, 0);
    for (const auto& s : students) {
        int b = (bins == 1) ? 0 : std::min(bins - 1, static_cast<int>((s.score - st.min) / width));
        (s.gender == "Female" ? female : male)[b]++;
    }
    int max_count = 1;
    for (int b = 0; b < bins; ++b) max_count = std::max({max_count, male[b], female[b]});

    Elements rows;
    for (int b = 0; b < bins; ++b) {
        std::string label = (bins == 1) ? fmt(st.min, 1)
                                        : fmt(st.min + b * width, 1) + " - " + fmt(st.min + (b + 1) * width, 1);
        rows.push_back(barRow(label + " M", static_cast<double>(male[b]) / max_count,
                              std::to_string(male[b]), MALE_COLOR, 22));
        rows.push_back(barRow("F", static_cast<double>(female[b]) / max_count,
                              std::to_string(female[b]), FEMALE_COLOR, 22));
    }

    return vbox({
        vbox(std::move(rows)) | vscroll_indicator | yframe | flex,
        separator(),
        hbox(text("Male") | color(MALE_COLOR), text("   "), text("Female") | color(FEMALE_COLOR),
             text("   " + std::to_string(bins) + (bins == 1 ? " range" : " ranges")) | dim),
    });
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

enum Page { MENU, ADD, MODIFY, REMOVE, VIEW, SEARCH, SORT, STATS, EXPORT };

int main() {
    auto screen = ScreenInteractive::Fullscreen();

    int page = MENU;

    auto goBack = [&] { page = MENU; };
    auto makeBackButton = [&] { return Button("Back", goBack, ButtonOption::Animated()); };

    std::vector<std::string> genders = {"Male", "Female"};

    // ------------------------------------------------ Add Student page ----
    std::string name_input, score_input, add_status;
    int add_gender = 0;
    bool add_ok = true;
    Component name_in;

    auto submitStudent = [&] {
        std::string name;
        double score = 0.0;
        if (!validateEntry(name_input, score_input, name, score, add_status)) {
            add_ok = false;
            return;
        }
        students.push_back({name, score, genders[add_gender]});
        add_status = "Added " + name + " (" + genders[add_gender] + ") with score " + fmt(score) + ".";
        add_ok = true;
        name_input.clear();
        score_input.clear();
        name_in->TakeFocus();  // ready for the next entry
    };

    InputOption score_opt;
    score_opt.on_enter = submitStudent;

    name_in = Input(&name_input, "e.g. Alice");
    auto add_gender_in = Toggle(&genders, &add_gender);
    auto score_in = Input(&score_input, "e.g. 87.5", score_opt);
    auto add_btn = Button("Add", submitStudent, ButtonOption::Animated());
    auto add_back = makeBackButton();

    auto add_form = Container::Vertical({
        name_in,
        add_gender_in,
        score_in,
        Container::Horizontal({add_btn, add_back}),
    });

    auto add_page = Renderer(add_form, [&] {
        return frame("Add Student",
                     vbox({
                         hbox(text("Name:   "), name_in->Render() | flex),
                         hbox(text("Gender: "), add_gender_in->Render()),
                         hbox(text("Score:  "), score_in->Render() | flex),
                         text(""),
                         hbox(add_btn->Render(), text(" "), add_back->Render()),
                         text(""),
                         text(add_status) | color(add_ok ? Color::Green : Color::Red),
                     }),
                     "Tab/arrows: move | Left/Right: change gender | Enter: submit | Esc: back to menu");
    });

    // -------------------------------------------- Modify Student page ----
    // Stage 0: pick a student.  Stage 1: edit the picked student.
    int modify_stage = 0;
    int modify_index = -1;
    std::vector<std::string> modify_entries;
    int modify_selected = 0;
    std::string mod_name, mod_score, modify_status;
    int mod_gender = 0;
    bool modify_ok = true;

    auto refreshModifyList = [&] {
        modify_entries.clear();
        for (const auto& s : students)
            modify_entries.push_back(s.name + "  [" + s.gender + "]  (" + fmt(s.score) + ")");
        if (modify_selected >= static_cast<int>(modify_entries.size()))
            modify_selected = std::max(0, static_cast<int>(modify_entries.size()) - 1);
    };

    // Stage 1 components (created first because the list's on_enter focuses one).
    auto mod_name_in = Input(&mod_name, "name");
    auto mod_gender_in = Toggle(&genders, &mod_gender);

    auto saveModification = [&] {
        if (modify_index < 0 || modify_index >= static_cast<int>(students.size())) return;
        std::string name;
        double score = 0.0;
        if (!validateEntry(mod_name, mod_score, name, score, modify_status)) {
            modify_ok = false;
            return;
        }
        students[modify_index].name = name;
        students[modify_index].score = score;
        students[modify_index].gender = genders[mod_gender];
        modify_status = "Updated " + name + ".";
        modify_ok = true;
        refreshModifyList();
        modify_stage = 0;
    };

    InputOption mod_score_opt;
    mod_score_opt.on_enter = saveModification;
    auto mod_score_in = Input(&mod_score, "score", mod_score_opt);
    auto mod_save = Button("Save", saveModification, ButtonOption::Animated());
    auto mod_cancel = Button("Cancel", [&] {
        modify_status.clear();
        modify_stage = 0;
    }, ButtonOption::Animated());

    auto modify_edit_container = Container::Vertical({
        mod_name_in,
        mod_gender_in,
        mod_score_in,
        Container::Horizontal({mod_save, mod_cancel}),
    });

    // Stage 0 components.
    MenuOption modify_opt = MenuOption::Vertical();
    modify_opt.on_enter = [&] {
        if (students.empty()) return;
        modify_index = modify_selected;
        const Student& s = students[modify_index];
        mod_name = s.name;
        mod_score = fmt(s.score);
        mod_gender = (s.gender == "Female") ? 1 : 0;
        modify_status.clear();
        modify_ok = true;
        modify_stage = 1;
        mod_name_in->TakeFocus();
    };
    auto modify_menu = Menu(&modify_entries, &modify_selected, modify_opt);
    auto modify_back = makeBackButton();
    auto modify_list_container = Container::Vertical({modify_menu, modify_back});

    auto modify_tab = Container::Tab({modify_list_container, modify_edit_container}, &modify_stage);

    auto modify_page = Renderer(modify_tab, [&] {
        if (modify_stage == 0) {
            Element body = students.empty() ? text("No students to modify.") | dim
                                            : modify_menu->Render() | vscroll_indicator | yframe;
            return frame("Modify Student",
                         vbox({body | flex, text(""),
                               text(modify_status) | color(modify_ok ? Color::Green : Color::Red),
                               modify_back->Render()}),
                         "Up/Down: select | Enter: edit | Esc: back to menu");
        }
        return frame("Edit Student",
                     vbox({
                         hbox(text("Name:   "), mod_name_in->Render() | flex),
                         hbox(text("Gender: "), mod_gender_in->Render()),
                         hbox(text("Score:  "), mod_score_in->Render() | flex),
                         text(""),
                         hbox(mod_save->Render(), text(" "), mod_cancel->Render()),
                         text(""),
                         text(modify_status) | color(modify_ok ? Color::Green : Color::Red),
                     }),
                     "Tab/arrows: move | Enter: save | Esc: back to list");
    });

    // ---------------------------------------------- Remove Student page ----
    std::vector<std::string> remove_entries;
    int remove_selected = 0;
    std::string remove_status;

    auto refreshRemoveList = [&] {
        remove_entries.clear();
        for (const auto& s : students)
            remove_entries.push_back(s.name + "  [" + s.gender + "]  (" + fmt(s.score) + ")");
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
            if (!found.name.empty()) {              // sentinel is {"", -1.0}
                results = vbox({text("Exact match:") | bold, studentTable({found})});
            } else {
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
    std::vector<std::string> stats_views = {"Summary", "Score Bars", "Distribution"};
    int stats_view = 0;
    auto stats_toggle = Toggle(&stats_views, &stats_view);
    auto stats_back = makeBackButton();
    auto stats_container = Container::Vertical({stats_toggle, stats_back});

    auto stats_page = Renderer(stats_container, [&] {
        Element body;
        if (students.empty()) {
            body = text("No data yet. Add some students first.") | dim;
        } else {
            Stats all = computeStats(students);
            if (stats_view == 0) {
                Stats m = computeStats(byGender("Male"));
                Stats f = computeStats(byGender("Female"));

                // Empty groups show "-" instead of misleading zeros.
                auto val = [](const Stats& s, double v) { return s.count ? fmt(v) : std::string("-"); };
                auto cell = [](const std::string& s) { return text(s) | size(WIDTH, EQUAL, 12); };

                auto header = hbox({text("") | size(WIDTH, EQUAL, 26),
                                    text("Overall") | bold | size(WIDTH, EQUAL, 12),
                                    text("Male") | bold | color(MALE_COLOR) | size(WIDTH, EQUAL, 12),
                                    text("Female") | bold | color(FEMALE_COLOR) | size(WIDTH, EQUAL, 12)});
                auto row = [&](const std::string& label, std::function<double(const Stats&)> get) {
                    return hbox({text(label) | size(WIDTH, EQUAL, 26), cell(val(all, get(all))) | bold,
                                 cell(val(m, get(m))) | bold, cell(val(f, get(f))) | bold});
                };

                Elements rulers;
                rulers.push_back(rangeRuler("All", all, Color::Cyan));
                if (m.count) rulers.push_back(rangeRuler("Male", m, MALE_COLOR));
                if (f.count) rulers.push_back(rangeRuler("Female", f, FEMALE_COLOR));
                rulers.push_back(text("● mean   ◆ median   ◉ both   ━ within 1 std dev of the mean") | dim);

                body = vbox({
                           header,
                           separator(),
                           hbox({text("Count:") | size(WIDTH, EQUAL, 26),
                                 cell(std::to_string(all.count)) | bold,
                                 cell(std::to_string(m.count)) | bold,
                                 cell(std::to_string(f.count)) | bold}),
                           row("Mean:", [](const Stats& s) { return s.mean; }),
                           row("Standard Deviation:", [](const Stats& s) { return s.std_dev; }),
                           row("Minimum Score:", [](const Stats& s) { return s.min; }),
                           row("Maximum Score:", [](const Stats& s) { return s.max; }),
                           row("50th Percentile (Median):", [](const Stats& s) { return s.median; }),
                           text(""),
                           text("Score range") | bold,
                           vbox(std::move(rulers)),
                       }) |
                       vscroll_indicator | yframe;
            } else if (stats_view == 1) {
                body = scoreBarsChart(all);
            } else {
                body = distributionChart(all);
            }
        }
        return frame("Statistics",
                     vbox({stats_toggle->Render() | hcenter, separator(), body | flex, text(""),
                           stats_back->Render()}),
                     "Left/Right: switch chart | Up/Down: move | Esc: back to menu");
    });

        // ------------------------------------------------------ Export ------
    std::string export_name = "students", export_dir, export_status;
    bool export_ok = true;
    std::vector<std::string> export_formats = {"CSV (.csv)", "Excel (.xlsx)"};
    int export_format = 0;

    // Removes spaces and the quotes Windows adds with "Copy as path".
    auto cleanPath = [](std::string s) {
        s = trim(s);
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                              (s.front() == '\'' && s.back() == '\'')))
            s = trim(s.substr(1, s.size() - 2));
        return s;
    };

    auto doExport = [&] {
        namespace fs = std::filesystem;

        if (students.empty()) {
            export_status = "No students to export.";
            export_ok = false;
            return;
        }

        // Expand a leading ~ to $HOME (std::filesystem does not do this).
        auto expandTilde = [](std::string s) {
            if (!s.empty() && s[0] == '~') {
                const char* home = std::getenv("HOME");
                if (home) s = std::string(home) + s.substr(1);
            }
            return s;
        };

        std::string base = expandTilde(cleanPath(export_name));
        if (base.empty()) {
            export_status = "File name cannot be empty.";
            export_ok = false;
            return;
        }

        std::string ext = (export_format == 0) ? ".csv" : ".xlsx";
        fs::path file(base);
        if (toLower(file.extension().string()) != ext) file += ext;

        // If the file name field already contains an absolute path,
        // use it directly and ignore the Folder field.
        fs::path target;
        std::string dir_text = expandTilde(cleanPath(export_dir));

        if (file.is_absolute()) {
            target = file;
        } else if (!dir_text.empty()) {
            std::error_code ec;
            fs::path dir(dir_text);
            if (!fs::is_directory(dir, ec)) {
                export_status = "Folder \"" + dir_text + "\" does not exist.";
                export_ok = false;
                return;
            }
            target = dir / file;
        } else {
            target = file;
        }

        // Create the parent directory if it's missing.
        std::error_code ec;
        fs::path parent = target.parent_path();
        if (!parent.empty() && !fs::exists(parent, ec)) {
            fs::create_directories(parent, ec);
            if (ec) {
                export_status = "Cannot create folder \"" + parent.string() + "\": " + ec.message();
                export_ok = false;
                return;
            }
        }

        std::string err;
        bool ok = (export_format == 0) ? writeCsv(target.string(), students, err)
                                    : writeXlsx(target.string(), students, err);
        if (ok) {
            std::error_code ec2;
            fs::path full = fs::absolute(target, ec2);
            export_status = "Exported " + std::to_string(students.size()) + " students to " +
                            (ec2 ? target.string() : full.string());
            export_ok = true;
        } else {
            export_status = err;
            export_ok = false;
        }
    };

    InputOption export_opt;
    export_opt.on_enter = doExport;
    auto export_name_in = Input(&export_name, "file name", export_opt);
    auto export_dir_in = Input(&export_dir, "empty = current folder", export_opt);
    auto export_format_in = Toggle(&export_formats, &export_format);
    auto export_btn = Button("Export", doExport, ButtonOption::Animated());
    auto export_back = makeBackButton();

    auto export_form = Container::Vertical({
        export_name_in,
        export_dir_in,
        export_format_in,
        Container::Horizontal({export_btn, export_back}),
    });

    auto export_page = Renderer(export_form, [&] {
        return frame("Export Students",
                     vbox({
                         hbox(text("File name: "), export_name_in->Render() | flex),
                         hbox(text("Folder:    "), export_dir_in->Render() | flex),
                         hbox(text("Format:    "), export_format_in->Render()),
                         text(""),
                         text("Columns: Name, Gender, Score  (" + std::to_string(students.size()) +
                              " students)") | dim,
                         text("Leave Folder empty to save where the program was started.") | dim,
                         text(""),
                         hbox(export_btn->Render(), text(" "), export_back->Render()),
                         text(""),
                         paragraph(export_status) | color(export_ok ? Color::Green : Color::Red),
                     }),
                     "Tab/arrows: move | Left/Right: change format | Enter: export | Esc: back to menu");
    });

    // -------------------------------------------------------- Menu --------
    std::vector<std::string> menu_items = {"Add Student",    "Modify Student", "Remove Student",
                                           "View All",       "Search by Name", "Sort by Score",
                                           "Statistics",     "Export",     "Exit"};
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
                modify_status.clear();
                modify_ok = true;
                modify_stage = 0;
                refreshModifyList();
                page = MODIFY;
                break;
            case 2:
                remove_status.clear();
                refreshRemoveList();
                page = REMOVE;
                break;
            case 3: page = VIEW; break;
            case 4:
                query.clear();
                page = SEARCH;
                query_in->TakeFocus();
                break;
            case 5: page = SORT; break;
            case 6:
                stats_view = 0;
                page = STATS;
                break;
            case 7:
                export_status.clear();
                export_ok = true;
                page = EXPORT;
                export_name_in->TakeFocus();
                break;
            case 8: screen.Exit(); break;
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
    // Order must match the Page enum.
        auto pages = Container::Tab({menu_page, add_page, modify_page, remove_page, view_page,
                                 search_page, sort_page, stats_page, export_page},
                                &page);

    auto root = CatchEvent(pages, [&](Event e) {
        if (e == Event::Escape && page != MENU) {
            if (page == MODIFY && modify_stage == 1) {  // leave the editor, not the page
                modify_stage = 0;
                modify_status.clear();
                return true;
            }
            page = MENU;
            return true;
        }
        return false;
    });

    screen.Loop(root);
    return 0;
}