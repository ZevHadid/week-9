//  ____________________________________________________________________________
// |                        Student Records Manager                             |
// |                          Elijah & Zev - 2026                               |
// |                                                                            |
// |  Credits & Third-Party Libraries                                           |
// |                                                                            |
// |  [FTXUI] by Arthur Sonzogni — MIT                                          |
// |____________________________________________________________________________|

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "helper_functions.h"
#include "app_core.h"

#include "surprise.h"
#include <cstdlib>
#include <ctime>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace ftxui;

// ---------------------------------------------------------------- data ----

struct Stats {
    size_t count = 0;
    double mean = 0.0, std_dev = 0.0;
    double min = 0.0, max = 0.0, median = 0.0;
};

Stats computeStats(const std::vector<Student>& list) {
    Stats st;
    if (list.empty()) return st;
    std::vector<double> v;
    v.reserve(list.size());
    for (const auto& s : list) v.push_back(s.score);
    st.count = v.size();
    st.mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    if (v.size() > 1) {
        double vs = 0.0;
        for (double x : v) vs += (x - st.mean) * (x - st.mean);
        st.std_dev = std::sqrt(vs / (v.size() - 1));
    }
    std::sort(v.begin(), v.end());
    st.min = v.front(); st.max = v.back();
    size_t n = v.size();
    st.median = (n % 2 == 1) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
    return st;
}

std::vector<Student> byGender(const std::string& g) {
    std::vector<Student> out;
    for (const auto& s : students) if (s.gender == g) out.push_back(s);
    return out;
}

// ------------------------------------------------------------- helpers ----

std::string fmt(double value, int precision = 2) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(precision) << value;
    return os.str();
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower((unsigned char)c));
    return s;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

const Color MALE_COLOR   = Color::BlueLight;
const Color FEMALE_COLOR = Color::Magenta;
Color genderColor(const std::string& g) { return g == "Female" ? FEMALE_COLOR : MALE_COLOR; }

Element studentTable(const std::vector<Student>& list) {
    if (list.empty()) return text("No students to display.") | dim;
    Elements rows;
    rows.push_back(hbox({
        text("#")      | size(WIDTH, EQUAL, 5),
        text("Name")   | size(WIDTH, EQUAL, 22),
        text("Gender") | size(WIDTH, EQUAL, 10),
        text("Score")  | size(WIDTH, EQUAL, 10),
    }) | bold);
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

Element barRow(const std::string& label, double frac, const std::string& value,
               Color c, int label_width = 16) {
    frac = std::clamp(frac, 0.0, 1.0);
    return hbox({
        text(label) | size(WIDTH, EQUAL, label_width),
        gauge(static_cast<float>(frac)) | color(c) | flex,
        text(" " + value) | size(WIDTH, EQUAL, 9),
    });
}

Element rangeRuler(const std::string& label, const Stats& st, Color c) {
    const int W = 48;
    double span = st.max - st.min;
    auto pos = [&](double v) {
        if (span <= 0.0) return 0;
        int p = (int)std::lround((v - st.min) / span * (W - 1));
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
        hbox(text("Male") | color(MALE_COLOR), text("   "),
             text("Female") | color(FEMALE_COLOR),
             text("   Overall mean: " + fmt(st.mean)) | dim),
    });
}

Element distributionChart(const Stats& st) {
    size_t n = students.size();
    int bins = (st.max == st.min)
        ? 1 : std::clamp((int)std::ceil(std::sqrt((double)n)), 3, 10);
    double width = (st.max - st.min) / bins;
    std::vector<int> male(bins, 0), female(bins, 0);
    for (const auto& s : students) {
        int b = (bins == 1) ? 0 : std::min(bins - 1, (int)((s.score - st.min) / width));
        (s.gender == "Female" ? female : male)[b]++;
    }
    int max_count = 1;
    for (int b = 0; b < bins; ++b) max_count = std::max({max_count, male[b], female[b]});
    Elements rows;
    for (int b = 0; b < bins; ++b) {
        std::string label = (bins == 1) ? fmt(st.min, 1)
            : fmt(st.min + b * width, 1) + " - " + fmt(st.min + (b + 1) * width, 1);
        rows.push_back(barRow(label + " M", (double)male[b] / max_count,
                              std::to_string(male[b]), MALE_COLOR, 22));
        rows.push_back(barRow("F", (double)female[b] / max_count,
                              std::to_string(female[b]), FEMALE_COLOR, 22));
    }
    return vbox({
        vbox(std::move(rows)) | vscroll_indicator | yframe | flex,
        separator(),
        hbox(text("Male") | color(MALE_COLOR), text("   "),
             text("Female") | color(FEMALE_COLOR),
             text("   " + std::to_string(bins) + (bins == 1 ? " range" : " ranges")) | dim),
    });
}

Element frame(const std::string& title, Element body, const std::string& hint) {
    return vbox({
        text(title) | bold | hcenter,
        separator(),
        body | flex,
        separator(),
        text(hint) | dim,
    }) | border;
}

// ---------------------------------------------------------------- GUI ----

enum Page { MENU, ADD, MODIFY, REMOVE, VIEW, SEARCH, SORT, STATS, EXPORT, IMPORT };

int displayUI() {
    auto screen = ScreenInteractive::Fullscreen();

    int page = MENU;
    auto goBack = [&] { page = MENU; };
    auto makeBackButton = [&] { return Button("Back", goBack, ButtonOption::Animated()); };

    std::vector<std::string> genders = {"Male", "Female"};
    std::string menu_status;
    bool menu_status_ok = true;

    // ------------------------------------------------ Add Student page ----
    std::string name_input, score_input, add_status;
    int add_gender = 0;
    bool add_ok = true;
    Component name_in;

    auto submitStudent = [&] {
        double score = 0.0;
        if (!core::parseScore(score_input, score, add_status)) { add_ok = false; return; }
        if (!core::addStudent(name_input, genders[add_gender], score, add_status)) {
            add_ok = false;
            return;
        }
        add_status = "Added " + trim(name_input) + " (" + genders[add_gender] +
                     ") with score " + fmt(score) + ".";
        add_ok = true;
        name_input.clear();
        score_input.clear();
        name_in->TakeFocus();
    };

    InputOption score_opt;
    score_opt.on_enter = submitStudent;

    name_in = Input(&name_input, "e.g. Alice");
    auto add_gender_in = Toggle(&genders, &add_gender);
    auto score_in = Input(&score_input, "e.g. 87.5", score_opt);
    auto add_btn = Button("Add", submitStudent, ButtonOption::Animated());
    auto add_back = makeBackButton();

    auto add_form = Container::Vertical({
        name_in, add_gender_in, score_in,
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
                     "Tab/arrows: move | Left/Right: change gender | Enter: submit | Esc: back");
    });

    // -------------------------------------------- Modify Student page ----
    int modify_stage = 0, modify_index = -1;
    std::vector<std::string> modify_entries;
    int modify_selected = 0;
    std::string mod_name, mod_score, modify_status;
    int mod_gender = 0;
    bool modify_ok = true;

    auto refreshModifyList = [&] {
        modify_entries.clear();
        for (const auto& s : students)
            modify_entries.push_back(s.name + "  [" + s.gender + "]  (" + fmt(s.score) + ")");
        if (modify_selected >= (int)modify_entries.size())
            modify_selected = std::max(0, (int)modify_entries.size() - 1);
    };

    auto mod_name_in   = Input(&mod_name, "name");
    auto mod_gender_in = Toggle(&genders, &mod_gender);

    auto saveModification = [&] {
        if (modify_index < 0 || modify_index >= (int)students.size()) return;
        double score = 0.0;
        if (!core::parseScore(mod_score, score, modify_status)) { modify_ok = false; return; }
        if (!core::modifyStudent((size_t)modify_index, mod_name, genders[mod_gender],
                                 score, modify_status)) {
            modify_ok = false;
            return;
        }
        modify_status = "Updated " + trim(mod_name) + ".";
        modify_ok = true;
        refreshModifyList();
        modify_stage = 0;
    };

    InputOption mod_score_opt;
    mod_score_opt.on_enter = saveModification;
    auto mod_score_in = Input(&mod_score, "score", mod_score_opt);
    auto mod_save     = Button("Save", saveModification, ButtonOption::Animated());
    auto mod_cancel   = Button("Cancel", [&] {
        modify_status.clear();
        modify_stage = 0;
    }, ButtonOption::Animated());

    auto modify_edit_container = Container::Vertical({
        mod_name_in, mod_gender_in, mod_score_in,
        Container::Horizontal({mod_save, mod_cancel}),
    });

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
    auto modify_tab = Container::Tab({modify_list_container, modify_edit_container},
                                     &modify_stage);

    auto modify_page = Renderer(modify_tab, [&] {
        if (modify_stage == 0) {
            Element body = students.empty() ? text("No students to modify.") | dim
                                            : modify_menu->Render() | vscroll_indicator | yframe;
            return frame("Modify Student",
                         vbox({body | flex, text(""),
                               text(modify_status) | color(modify_ok ? Color::Green : Color::Red),
                               modify_back->Render()}),
                         "Up/Down: select | Enter: edit | Esc: back");
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
        if (remove_selected >= (int)remove_entries.size())
            remove_selected = std::max(0, (int)remove_entries.size() - 1);
    };

    MenuOption remove_opt = MenuOption::Vertical();
    remove_opt.on_enter = [&] {
        if (students.empty()) return;
        std::string gone = students[remove_selected].name;
        std::string err;
        if (core::removeStudent((size_t)remove_selected, err)) {
            remove_status = "Removed " + gone + ".";
        } else {
            remove_status = "Error: " + err;
        }
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
                     "Up/Down: select | Enter: remove | Esc: back");
    });

    // ------------------------------------------------- View / Search -------
    auto view_back = makeBackButton();
    auto view_page = Renderer(view_back, [&] {
        return frame("All Students",
                     vbox({studentTable(students) | vscroll_indicator | yframe | flex,
                           text(""), view_back->Render()}),
                     "Esc: back");
    });

    std::string query;
    auto query_in = Input(&query, "type part of a name...");
    auto search_back = makeBackButton();
    auto search_container = Container::Vertical({query_in, search_back});
    auto search_page = Renderer(search_container, [&] {
        std::string q = trim(query);
        Element results;
        if (q.empty()) results = text("Start typing a name to search.") | dim;
        else {
            Student found = findStudentNByName(q);
            if (!found.name.empty()) results = vbox({text("Exact match:") | bold,
                                                     studentTable({found})});
            else {
                std::vector<Student> similar;
                std::string lq = toLower(q);
                for (const auto& s : students)
                    if (toLower(s.name).find(lq) != std::string::npos) similar.push_back(s);
                if (similar.empty())
                    results = text("No student named \"" + q + "\".") | dim;
                else
                    results = vbox({text("No exact match. Similar names:") | dim,
                                    studentTable(similar)});
            }
        }
        return frame("Search by Name",
                     vbox({hbox(text("Search: "), query_in->Render() | flex), separator(),
                           results | vscroll_indicator | yframe | flex, text(""),
                           search_back->Render()}),
                     "Results update as you type | Esc: back");
    });

    bool descending = true;
    auto flip_btn = Button("Flip order", [&] { descending = !descending; },
                           ButtonOption::Animated());
    auto sort_back = makeBackButton();
    auto sort_container = Container::Horizontal({flip_btn, sort_back});
    auto sort_page = Renderer(sort_container, [&] {
        std::vector<int> order = sortedStudentsByScore();
        if (!descending) std::reverse(order.begin(), order.end());
        std::vector<Student> sorted;
        for (int idx : order) sorted.push_back(students[idx]);
        return frame("Sort by Score",
                     vbox({text(descending ? "Order: highest first" : "Order: lowest first") | bold,
                           separator(), studentTable(sorted) | vscroll_indicator | yframe | flex,
                           text(""), hbox(flip_btn->Render(), text(" "), sort_back->Render())}),
                     "Left/Right: move | Enter: press | Esc: back");
    });

    // ---------------------------------------------------- Statistics ------
    std::vector<std::string> stats_views = {"Summary", "Score Bars", "Distribution"};
    int stats_view = 0;
    auto stats_toggle = Toggle(&stats_views, &stats_view);
    auto stats_back = makeBackButton();
    auto stats_container = Container::Vertical({stats_toggle, stats_back});
    auto stats_page = Renderer(stats_container, [&] {
        Element body;
        if (students.empty()) body = text("No data yet. Add some students first.") | dim;
        else {
            Stats all = computeStats(students);
            if (stats_view == 0) {
                Stats m = computeStats(byGender("Male"));
                Stats f = computeStats(byGender("Female"));
                auto val = [](const Stats& s, double v) { return s.count ? fmt(v) : std::string("-"); };
                auto cell = [](const std::string& s) { return text(s) | size(WIDTH, EQUAL, 12); };
                auto header = hbox({text("") | size(WIDTH, EQUAL, 26),
                                    text("Overall") | bold | size(WIDTH, EQUAL, 12),
                                    text("Male")   | bold | color(MALE_COLOR)   | size(WIDTH, EQUAL, 12),
                                    text("Female") | bold | color(FEMALE_COLOR) | size(WIDTH, EQUAL, 12)});
                auto row = [&](const std::string& label, std::function<double(const Stats&)> get) {
                    return hbox({text(label) | size(WIDTH, EQUAL, 26),
                                 cell(val(all, get(all))) | bold,
                                 cell(val(m, get(m))) | bold,
                                 cell(val(f, get(f))) | bold});
                };
                Elements rulers;
                rulers.push_back(rangeRuler("All", all, Color::Cyan));
                if (m.count) rulers.push_back(rangeRuler("Male", m, MALE_COLOR));
                if (f.count) rulers.push_back(rangeRuler("Female", f, FEMALE_COLOR));
                rulers.push_back(text("● mean  ◆ median  ◉ both  ━ within 1 std dev") | dim);
                body = vbox({
                    header, separator(),
                    hbox({text("Count:") | size(WIDTH, EQUAL, 26),
                          cell(std::to_string(all.count)) | bold,
                          cell(std::to_string(m.count)) | bold,
                          cell(std::to_string(f.count)) | bold}),
                    row("Mean:",                   [](const Stats& s){ return s.mean; }),
                    row("Standard Deviation:",     [](const Stats& s){ return s.std_dev; }),
                    row("Minimum Score:",          [](const Stats& s){ return s.min; }),
                    row("Maximum Score:",          [](const Stats& s){ return s.max; }),
                    row("50th Percentile (Median):",[](const Stats& s){ return s.median; }),
                    text(""), text("Score range") | bold,
                    vbox(std::move(rulers)),
                }) | vscroll_indicator | yframe;
            } else if (stats_view == 1) body = scoreBarsChart(all);
            else                        body = distributionChart(all);
        }
        return frame("Statistics",
                     vbox({stats_toggle->Render() | hcenter, separator(), body | flex, text(""),
                           stats_back->Render()}),
                     "Left/Right: switch chart | Esc: back");
    });

    // ---------------------------------------- shared path utilities ------
    auto cleanPath = [](std::string s) {
        s = trim(s);
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                              (s.front() == '\'' && s.back() == '\'')))
            s = trim(s.substr(1, s.size() - 2));
        return s;
    };
    auto expandTilde = [](std::string s) {
        if (!s.empty() && s[0] == '~') {
            const char* home = std::getenv("HOME");
            if (home) s = std::string(home) + s.substr(1);
        }
        return s;
    };
    auto resolvePath = [&](const std::string& raw) {
        return expandTilde(cleanPath(raw));
    };

    // ------------------------------------------------------ Export ------
    std::string export_name = "students", export_dir, export_status;
    bool export_ok = true;
    std::vector<std::string> export_formats = {"CSV (.csv)", "Excel (.xlsx)"};
    int export_format = 0;

    auto doExport = [&] {
        namespace fs = std::filesystem;
        if (students.empty()) { export_status = "No students to export."; export_ok = false; return; }

        std::string base = resolvePath(export_name);
        if (base.empty()) { export_status = "File name cannot be empty."; export_ok = false; return; }

        std::string ext = (export_format == 0) ? ".csv" : ".xlsx";
        fs::path file(base);
        if (toLower(file.extension().string()) != ext) file += ext;

        fs::path target;
        std::string dir_text = resolvePath(export_dir);
        if (file.is_absolute())      target = file;
        else if (!dir_text.empty()) {
            std::error_code ec;
            fs::path dir(dir_text);
            if (!fs::is_directory(dir, ec)) {
                export_status = "Folder \"" + dir_text + "\" does not exist.";
                export_ok = false;
                return;
            }
            target = dir / file;
        } else target = file;

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
        } else { export_status = err; export_ok = false; }
    };

    InputOption export_opt;
    export_opt.on_enter = doExport;
    auto export_name_in   = Input(&export_name, "file name", export_opt);
    auto export_dir_in    = Input(&export_dir, "empty = current folder", export_opt);
    auto export_format_in = Toggle(&export_formats, &export_format);
    auto export_btn       = Button("Export", doExport, ButtonOption::Animated());
    auto export_back      = makeBackButton();
    auto export_form = Container::Vertical({
        export_name_in, export_dir_in, export_format_in,
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
                         text(""), hbox(export_btn->Render(), text(" "), export_back->Render()),
                         text(""), paragraph(export_status) | color(export_ok ? Color::Green : Color::Red),
                     }),
                     "Tab/arrows: move | Left/Right: change format | Enter: export | Esc: back");
    });

    // ------------------------------------------------------ Import ------
    std::string import_path, import_status;
    bool import_ok = true;
    std::vector<std::string> import_modes = {"Replace all", "Append"};
    int import_mode = 0;

    auto doImport = [&] {
        if (import_path.empty()) { import_status = "Enter a file path first."; import_ok = false; return; }
        std::string resolved = resolvePath(import_path);
        std::string lower = toLower(resolved);
        std::string ext;
        auto dot = lower.find_last_of('.');
        if (dot != std::string::npos) ext = lower.substr(dot);

        std::vector<Student> loaded;
        std::string err;
        bool ok = false;
        if (ext == ".csv")       ok = readCsv(resolved, loaded, err);
        else if (ext == ".xlsx") ok = readXlsx(resolved, loaded, err);
        else { import_status = "File must end in .csv or .xlsx."; import_ok = false; return; }
        if (!ok) { import_status = err; import_ok = false; return; }

        size_t oldSize = students.size();
        if (import_mode == 0) students = std::move(loaded);
        else                  students.insert(students.end(), loaded.begin(), loaded.end());
        core::session.dirty = true;  // in-memory state changed

        std::ostringstream msg;
        if (import_mode == 0)
            msg << "Replaced " << oldSize << " with " << loaded.size() << " imported students.";
        else
            msg << "Appended " << loaded.size() << " students (total " << students.size() << ").";
        import_status = msg.str();
        import_ok = true;
    };

    InputOption import_opt;
    import_opt.on_enter = doImport;
    auto import_path_in = Input(&import_path, "/path/to/students.csv", import_opt);
    auto import_mode_in = Toggle(&import_modes, &import_mode);
    auto import_btn     = Button("Import", doImport, ButtonOption::Animated());
    auto import_back    = makeBackButton();
    auto import_form = Container::Vertical({
        import_path_in, import_mode_in,
        Container::Horizontal({import_btn, import_back}),
    });
    auto import_page = Renderer(import_form, [&] {
        return frame("Import Students",
                     vbox({
                         hbox(text("Path:    "), import_path_in->Render() | flex),
                         hbox(text("Mode:    "), import_mode_in->Render()),
                         text(""),
                         text("Supports .csv and .xlsx (from this app's Export).") | dim,
                         text("Columns: Name, Gender, Score  — header row optional.") | dim,
                         text("~ is expanded to your home directory.") | dim,
                         text(""),
                         hbox(import_btn->Render(), text(" "), import_back->Render()),
                         text(""),
                         paragraph(import_status) | color(import_ok ? Color::Green : Color::Red),
                     }),
                     "Tab/arrows: move | Enter: import | Esc: back");
    });

    // -------------------------------------------------------- Menu --------
    std::vector<std::string> menu_items = {
        "Add Student", "Modify Student", "Remove Student",
        "View All", "Search by Name", "Sort by Score",
        "Statistics", "Export", "Import", "Save", "Exit"
    };
    int menu_selected = 0;

    MenuOption menu_opt = MenuOption::Vertical();
    menu_opt.on_enter = [&] {
        switch (menu_selected) {
            case 0:
                name_input.clear(); score_input.clear();
                add_status.clear(); add_ok = true;
                page = ADD; name_in->TakeFocus(); break;
            case 1:
                modify_status.clear(); modify_ok = true; modify_stage = 0;
                refreshModifyList(); page = MODIFY; break;
            case 2:
                remove_status.clear(); refreshRemoveList(); page = REMOVE; break;
            case 3: page = VIEW; break;
            case 4:
                query.clear(); page = SEARCH; query_in->TakeFocus(); break;
            case 5: page = SORT; break;
            case 6: stats_view = 0; page = STATS; break;
            case 7:
                export_status.clear(); export_ok = true;
                page = EXPORT; export_name_in->TakeFocus(); break;
            case 8:
                import_status.clear(); import_ok = true;
                page = IMPORT; import_path_in->TakeFocus(); break;
            case 9: {
                // Manual save
                std::string err;
                if (!core::session.bound) {
                    menu_status = "No file bound. Launch with: ./app gui <file>";
                    menu_status_ok = false;
                } else if (core::saveSession(err)) {
                    menu_status = "Saved to " + core::session.path;
                    menu_status_ok = true;
                } else {
                    menu_status = "Save failed: " + err;
                    menu_status_ok = false;
                }
                break;
            }
            case 10: screen.Exit(); break;
        }
    };
    auto menu = Menu(&menu_items, &menu_selected, menu_opt);

    auto sessionLine = [] {
        if (!core::session.bound) return std::string("No file bound (use './app gui <file>')");
        return std::string("File: ") + core::session.path +
               (core::session.dirty ? "  [unsaved]" : "  [saved]");
    };

    auto menu_page = Renderer(menu, [&] {
        Color sc = Color::Yellow;
        if (!core::session.bound) sc = Color::Red;
        else if (!core::session.dirty) sc = Color::Green;
        return frame("Student Records Manager",
                     vbox({
                         menu->Render() | hcenter,
                         text(""),
                         text("Students loaded: " + std::to_string(students.size())) | dim | hcenter,
                         text(sessionLine()) | color(sc) | hcenter,
                         text(menu_status) | color(menu_status_ok ? Color::Green : Color::Red) | hcenter,
                     }),
                     "Up/Down: navigate | Enter: select");
    });

    auto pages = Container::Tab({menu_page, add_page, modify_page, remove_page, view_page,
                                 search_page, sort_page, stats_page, export_page, import_page},
                                &page);

    auto root = CatchEvent(pages, [&](Event e) {
        if (e == Event::Escape && page != MENU) {
            if (page == MODIFY && modify_stage == 1) {
                modify_stage = 0; modify_status.clear();
                return true;
            }
            page = MENU;
            return true;
        }
        return false;
    });

    screen.Loop(root);

    // Auto-save on exit if dirty and bound.
    if (core::session.bound && core::session.dirty) {
        std::string err;
        if (!core::saveSession(err)) {
            std::cerr << "Auto-save failed: " << err << "\n";
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------- CLI ----

namespace {

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string cur;
    bool inQuote = false;
    for (char c : line) {
        if (c == '"')              inQuote = !inQuote;
        else if (c == ' ' && !inQuote) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else                     cur += c;
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

bool parseStudentArgs(const std::vector<std::string>& t, size_t nameStart,
                      std::string& name, std::string& gender, double& score,
                      std::string& err) {
    if (t.size() < nameStart + 3) {
        err = "Not enough arguments (need: <name...> <m|f> <score>).";
        return false;
    }
    size_t n = t.size();
    std::string scoreStr  = t[n - 1];
    std::string genderStr = t[n - 2];

    name.clear();
    for (size_t i = nameStart; i < n - 2; ++i) {
        if (!name.empty()) name += ' ';
        name += t[i];
    }

    gender = core::normalizeGender(genderStr);
    if (gender.empty()) {
        err = "Gender must be m or f (or Male/Female).";
        return false;
    }
    return core::parseScore(scoreStr, score, err);
}

void printUsage() {
    std::cout <<
        "Usage: app <command> [args...]\n\n"
        "  gui  [<file>]                                  Launch GUI (file binds session).\n"
        "  shell <file>                                   Interactive session (creates if missing).\n"
        "  add    <file> \"Full Name\" <m|f> <score>        Append, save, exit.\n"
        "  modify <file> <idx> \"Full Name\" <m|f> <score>\n"
        "  remove <file> <idx>\n"
        "  list   <file>\n"
        "  validate <file>\n"
        "  import <file> <source>\n"
        "  export <file> <dest>\n\n"
        "Quotes around the name are optional — names with spaces work either way.\n"
        "Gender is m or f (Male/Female also accepted). Indices are 1-based.\n";
}

// Bind to an existing file, run `op`, save, unbind. Returns process exit code.
int withSession(const std::string& file, const std::function<bool(std::string&)>& op) {
    namespace fs = std::filesystem;
    if (!fs::exists(file)) {
        std::cerr << "Error: file does not exist: " << file << "\n";
        return 1;
    }
    std::string err;
    if (!core::openSession(file, err)) {
        std::cerr << "Error opening file: " << err << "\n";
        return 1;
    }
    if (!op(err)) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }
    if (!core::saveSession(err)) {
        std::cerr << "Error saving: " << err << "\n";
        return 1;
    }
    return 0;
}

int cmdValidate(const std::string& file) {
    std::string err;
    if (!core::validateFile(file, err)) {
        std::cerr << "Invalid: " << err << "\n";
        return 1;
    }
    std::cout << "OK: " << file << " is a valid student file.\n";
    return 0;
}

int cmdList(const std::string& file) {
    std::string err;
    if (!core::openSession(file, err)) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }
    if (students.empty()) {
        std::cout << "(empty)\n";
    } else {
        std::cout << std::left
                  << std::setw(5)  << "#"
                  << std::setw(24) << "Name"
                  << std::setw(10) << "Gender"
                  << "Score\n";
        std::cout << std::string(50, '-') << "\n";
        for (size_t i = 0; i < students.size(); ++i) {
            std::cout << std::left
                      << std::setw(5)  << (i + 1)
                      << std::setw(24) << students[i].name
                      << std::setw(10) << students[i].gender
                      << fmt(students[i].score) << "\n";
        }
    }
    core::closeSession();
    return 0;
}

int cmdImport(const std::string& dst, const std::string& src) {
    std::string err;
    if (!core::openSession(dst, err)) {
        std::cerr << "Error opening destination: " << err << "\n";
        return 1;
    }
    std::vector<Student> loaded;
    std::string lower = toLower(src);
    bool ok = false;
    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".csv")
        ok = readCsv(src, loaded, err);
    else if (lower.size() >= 5 && lower.substr(lower.size() - 5) == ".xlsx")
        ok = readXlsx(src, loaded, err);
    else { std::cerr << "Source must be .csv or .xlsx.\n"; return 1; }
    if (!ok) { std::cerr << "Error reading source: " << err << "\n"; return 1; }

    students.insert(students.end(), loaded.begin(), loaded.end());
    core::session.dirty = true;
    if (!core::saveSession(err)) {
        std::cerr << "Error saving: " << err << "\n";
        return 1;
    }
    std::cout << "Imported " << loaded.size() << " students; total " << students.size() << ".\n";
    core::closeSession();
    return 0;
}

int cmdExport(const std::string& src, const std::string& dst) {
    std::string err;
    if (!core::openSession(src, err)) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }
    std::string lower = toLower(dst);
    bool ok;
    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".csv")
        ok = writeCsv(dst, students, err);
    else if (lower.size() >= 5 && lower.substr(lower.size() - 5) == ".xlsx")
        ok = writeXlsx(dst, students, err);
    else { std::cerr << "Destination must be .csv or .xlsx.\n"; return 1; }
    if (!ok) { std::cerr << "Error: " << err << "\n"; return 1; }
    std::cout << "Exported " << students.size() << " students to " << dst << "\n";
    core::closeSession();
    return 0;
}

int cmdShell(const std::string& file) {
    namespace fs = std::filesystem;
    std::string err;

    if (fs::exists(file)) {
        if (!core::openSession(file, err)) {
            std::cerr << "Error opening existing file: " << err << "\n";
            return 1;
        }
        std::cout << "Opened " << file << " (" << students.size() << " students).\n";
    } else {
        if (!core::newSession(file, false, err)) {
            std::cerr << "Error creating file: " << err << "\n";
            return 1;
        }
        std::cout << "Created new session bound to " << file << ".\n";
    }
    std::cout << "Type 'help' for commands, 'logout' to save and exit.\n\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        auto t = tokenize(line);
        if (t.empty()) continue;
        const std::string& cmd = t[0];

        if (cmd == "help") {
            std::cout <<
                "  add \"First Last\" m|f <score>      Append a student\n"
                "  modify <idx> \"First Last\" m|f <score>\n"
                "  remove <idx>\n"
                "  list\n"
                "  status\n"
                "  save\n"
                "  logout                            Save and exit\n"
                "  (quotes optional — names with spaces work either way)\n";
        } else if (cmd == "list") {
            if (students.empty()) std::cout << "(empty)\n";
            else {
                std::cout << std::left
                          << std::setw(5) << "#" << std::setw(24) << "Name"
                          << std::setw(10) << "Gender" << "Score\n"
                          << std::string(50, '-') << "\n";
                for (size_t i = 0; i < students.size(); ++i)
                    std::cout << std::left << std::setw(5) << (i + 1)
                              << std::setw(24) << students[i].name
                              << std::setw(10) << students[i].gender
                              << fmt(students[i].score) << "\n";
            }
        } else if (cmd == "add") {
            std::string name, gender, e2;
            double score = 0.0;
            if (!parseStudentArgs(t, 1, name, gender, score, e2)) {
                std::cout << "Error: " << e2 << "\n"
                        << "Usage: add \"First Last\" m|f <score>\n";
                continue;
            }
            if (!core::addStudent(name, gender, score, e2)) std::cout << "Error: " << e2 << "\n";
            else std::cout << "Added.\n";
        } else if (cmd == "modify") {
            if (t.size() < 5) {
                std::cout << "Usage: modify <idx> \"First Last\" m|f <score>\n";
                continue;
            }
            size_t idx = 0;
            try { idx = std::stoul(t[1]); } catch (...) { std::cout << "Bad index.\n"; continue; }
            if (idx == 0) { std::cout << "Indices are 1-based.\n"; continue; }
            std::string name, gender, e2;
            double score = 0.0;
            if (!parseStudentArgs(t, 2, name, gender, score, e2)) {
                std::cout << "Error: " << e2 << "\n";
                continue;
            }
            if (!core::modifyStudent(idx - 1, name, gender, score, e2))
                std::cout << "Error: " << e2 << "\n";
            else std::cout << "Modified.\n";
        } else if (cmd == "remove") {
            if (t.size() < 2) { std::cout << "Usage: remove <idx>\n"; continue; }
            size_t idx = 0;
            try { idx = std::stoul(t[1]); } catch (...) { std::cout << "Bad index.\n"; continue; }
            if (idx == 0) { std::cout << "Indices are 1-based.\n"; continue; }
            if (!core::removeStudent(idx - 1, err)) std::cout << "Error: " << err << "\n";
            else std::cout << "Removed.\n";
        } else if (cmd == "status") {
            std::cout << "File: " << core::session.path
                      << (core::session.dirty ? "  [unsaved]" : "  [saved]") << "\n"
                      << "Students: " << students.size() << "\n";
        } else if (cmd == "save") {
            if (core::saveSession(err)) std::cout << "Saved.\n";
            else std::cout << "Error: " << err << "\n";
        } else if (cmd == "logout" || cmd == "exit" || cmd == "quit") {
            break;
        } else {
            std::cout << "Unknown command '" << cmd << "'. Type 'help'.\n";
        }

        if (std::rand() % 100 < 30) {
            std::cout << "\n[!] Wait... what's this? Opening a mysterious window...\n";
            showSurpriseImage();
            std::cout << "[!] Window closed. Continuing...\n\n";
        }
    }

    if (core::session.dirty) {
        if (core::saveSession(err)) std::cout << "\nSaved to " << core::session.path << "\n";
        else { std::cerr << "Error saving: " << err << "\n"; return 1; }
    } else {
        std::cout << "\nNothing to save.\n";
    }
    core::closeSession();
    return 0;
}

} // namespace

// ---------------------------------------------------------------- main ----

int main(int argc, char* argv[]) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) { printUsage(); return 1; }

    const std::string& cmd = args[0];

    if (cmd == "gui") {
        if (args.size() >= 2) {
            std::string file = args[1];
            namespace fs = std::filesystem;
            std::string err;
            if (fs::exists(file)) {
                if (!core::openSession(file, err)) {
                    std::cerr << "Error: " << err << "\n";
                    return 1;
                }
            } else {
                if (!core::newSession(file, false, err)) {
                    std::cerr << "Error: " << err << "\n";
                    return 1;
                }
            }
        }
        return displayUI();
    }

    if (cmd == "shell") {
        if (args.size() < 2) { printUsage(); return 1; }
        return cmdShell(args[1]);
    }

    if (cmd == "validate") {
        if (args.size() < 2) { printUsage(); return 1; }
        return cmdValidate(args[1]);
    }

    if (cmd == "list") {
        if (args.size() < 2) { printUsage(); return 1; }
        return cmdList(args[1]);
    }

    if (cmd == "add") {
        if (args.size() < 5) { printUsage(); return 1; }
        std::string name, gender, err;
        double score = 0.0;
        if (!parseStudentArgs(args, 2, name, gender, score, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        return withSession(args[1], [&](std::string& e) {
            return core::addStudent(name, gender, score, e);
        });
    }

    if (cmd == "modify") {
        if (args.size() < 6) { printUsage(); return 1; }
        size_t idx = 0;
        try { idx = std::stoul(args[2]); } catch (...) {
            std::cerr << "Bad index.\n"; return 1;
        }
        if (idx == 0) { std::cerr << "Indices are 1-based.\n"; return 1; }

        std::string name, gender, err;
        double score = 0.0;
        if (!parseStudentArgs(args, 3, name, gender, score, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        return withSession(args[1], [&](std::string& e) {
            return core::modifyStudent(idx - 1, name, gender, score, e);
        });
    }

    if (cmd == "remove") {
        if (args.size() < 3) { printUsage(); return 1; }
        size_t idx = 0;
        try { idx = std::stoul(args[2]); } catch (...) {
            std::cerr << "Bad index.\n"; return 1;
        }
        if (idx == 0) { std::cerr << "Indices are 1-based.\n"; return 1; }
        return withSession(args[1], [&](std::string& e) {
            return core::removeStudent(idx - 1, e);
        });
    }

    if (cmd == "import") {
        if (args.size() < 3) { printUsage(); return 1; }
        return cmdImport(args[1], args[2]);
    }

    if (cmd == "export") {
        if (args.size() < 3) { printUsage(); return 1; }
        return cmdExport(args[1], args[2]);
    }

    std::cerr << "Unknown command: " << cmd << "\n\n";
    printUsage();
    return 1;
}