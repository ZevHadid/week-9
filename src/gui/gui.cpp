#include "gui.h"
#include "charts.h"
#include "widgets.h"

#include "core/validators.h"
#include "util/path.h"
#include "util/str.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace ftxui;
using util::fmt;
using util::trim;
using util::toLower;

namespace gui {

namespace {

enum Page { MENU, ADD, MODIFY, REMOVE, VIEW, SEARCH, SORT, STATS, EXPORT, IMPORT };

std::string paddedLabel(const std::string& label, size_t width = 14) {
    std::string out = label + ":";
    while (out.size() < width) out += ' ';
    return out;
}

bool pathIsXlsx(const std::string& path) {
    return toLower(std::filesystem::path(path).extension().string()) == ".xlsx";
}

}

int run(core::Store& store) {
    auto screen = ScreenInteractive::Fullscreen();

    int page = MENU;
    auto goBack = [&] { page = MENU; };
    auto makeBackButton = [&] { return Button("Back", goBack, ButtonOption::Animated()); };

    std::vector<std::string> genders = {"Male", "Female"};
    std::string menu_status;
    bool menu_status_ok = true;

    // ------------------------------------------------------------- ADD ----
    std::string add_grade_year, add_birth_date, add_name, add_status;
    std::vector<std::string> add_grades(SUBJECTS.size());
    int add_gender = 0;
    bool add_ok = true;
    Component add_grade_year_in, add_birth_date_in, add_name_in;
    std::vector<Component> add_grade_inputs;

    auto submitStudent = [&] {
        std::vector<double> gs;
        for (const auto& g : add_grades) {
            double d = 0.0;
            if (!core::parseGrade(g, d, add_status)) { add_ok = false; return; }
            gs.push_back(d);
        }
        Student s;
        s.grade_year = add_grade_year;
        s.birth_date = add_birth_date;
        s.full_name  = add_name;
        s.gender     = genders[add_gender];
        s.grades     = gs;
        if (!store.add(s, add_status)) { add_ok = false; return; }
        add_status = "Added " + trim(add_name) + ".";
        add_ok = true;
        add_grade_year.clear();
        add_birth_date.clear();
        add_name.clear();
        for (auto& g : add_grades) g.clear();
        add_grade_year_in->TakeFocus();
    };

    std::vector<InputOption> grade_opts(SUBJECTS.size());
    grade_opts.back().on_enter = submitStudent;
    for (size_t i = 0; i < SUBJECTS.size(); ++i)
        add_grade_inputs.push_back(Input(&add_grades[i], "0-100", grade_opts[i]));

    add_grade_year_in = Input(&add_grade_year, "e.g. Freshman");
    add_birth_date_in = Input(&add_birth_date, "YYYY-MM-DD");
    add_name_in       = Input(&add_name, "e.g. Alice Johnson");
    auto add_gender_in = Toggle(&genders, &add_gender);
    auto add_btn  = Button("Add", submitStudent, ButtonOption::Animated());
    auto add_back = makeBackButton();

    std::vector<Component> add_children = {
        add_grade_year_in, add_birth_date_in, add_name_in, add_gender_in,
    };
    for (auto& inp : add_grade_inputs) add_children.push_back(inp);
    add_children.push_back(Container::Horizontal({add_btn, add_back}));
    auto add_form = Container::Vertical(add_children);

    auto add_page = Renderer(add_form, [&] {
        Elements body;
        body.push_back(hbox(text(paddedLabel("Grade Year")) | size(WIDTH, EQUAL, 14),
                            add_grade_year_in->Render() | flex));
        body.push_back(hbox(text(paddedLabel("Birth Date")) | size(WIDTH, EQUAL, 14),
                            add_birth_date_in->Render() | flex));
        body.push_back(hbox(text(paddedLabel("Full Name")) | size(WIDTH, EQUAL, 14),
                            add_name_in->Render() | flex));
        body.push_back(hbox(text(paddedLabel("Gender")) | size(WIDTH, EQUAL, 14),
                            add_gender_in->Render()));
        body.push_back(separator());
        for (size_t i = 0; i < SUBJECTS.size(); ++i)
            body.push_back(hbox(text(paddedLabel(SUBJECTS[i])) | size(WIDTH, EQUAL, 14),
                                add_grade_inputs[i]->Render() | flex));
        body.push_back(text(""));
        body.push_back(hbox(add_btn->Render(), text(" "), add_back->Render()));
        body.push_back(text(""));
        body.push_back(text(add_status) | color(add_ok ? Color::Green : Color::Red));

        return borderedPanel("Add Student",
                             vbox(std::move(body)) | vscroll_indicator | yframe | flex,
                             "Tab/arrows: move | Enter on last field: submit | Esc: back");
    });

    // ---------------------------------------------------------- MODIFY ----
    int modify_stage = 0, modify_index = -1;
    std::vector<std::string> modify_entries;
    int modify_selected = 0;
    std::string mod_grade_year, mod_birth_date, mod_name, mod_status;
    std::vector<std::string> mod_grades(SUBJECTS.size());
    int mod_gender = 0;
    bool modify_ok = true;
    std::vector<Component> mod_grade_inputs;

    for (size_t i = 0; i < SUBJECTS.size(); ++i)
        mod_grade_inputs.push_back(Input(&mod_grades[i], "0-100"));

    auto refreshModifyList = [&] {
        modify_entries.clear();
        for (const auto& s : store.all())
            modify_entries.push_back(s.full_name + "  [" + s.gender + "]  (" +
                                     s.grade_year + ")  avg " + averageString(s));
        if (modify_selected >= static_cast<int>(modify_entries.size()))
            modify_selected = std::max(0, static_cast<int>(modify_entries.size()) - 1);
    };

    auto mod_grade_year_in = Input(&mod_grade_year, "");
    auto mod_birth_date_in = Input(&mod_birth_date, "YYYY-MM-DD");
    auto mod_name_in       = Input(&mod_name, "");
    auto mod_gender_in     = Toggle(&genders, &mod_gender);

    auto saveModification = [&] {
        if (modify_index < 0 || modify_index >= static_cast<int>(store.size())) return;
        std::vector<double> gs;
        for (const auto& g : mod_grades) {
            double d = 0.0;
            if (!core::parseGrade(g, d, mod_status)) { modify_ok = false; return; }
            gs.push_back(d);
        }
        Student s;
        s.grade_year = mod_grade_year;
        s.birth_date = mod_birth_date;
        s.full_name  = mod_name;
        s.gender     = genders[mod_gender];
        s.grades     = gs;
        if (!store.modify(static_cast<size_t>(modify_index), s, mod_status)) {
            modify_ok = false;
            return;
        }
        mod_status = "Updated " + trim(mod_name) + ".";
        modify_ok = true;
        refreshModifyList();
        modify_stage = 0;
    };

    auto mod_save   = Button("Save", saveModification, ButtonOption::Animated());
    auto mod_cancel = Button("Cancel", [&] {
        mod_status.clear();
        modify_stage = 0;
    }, ButtonOption::Animated());

    std::vector<Component> mod_edit_children = {
        mod_grade_year_in, mod_birth_date_in, mod_name_in, mod_gender_in,
    };
    for (auto& inp : mod_grade_inputs) mod_edit_children.push_back(inp);
    mod_edit_children.push_back(Container::Horizontal({mod_save, mod_cancel}));

    auto modify_edit_container = Container::Vertical(mod_edit_children);

    MenuOption modify_opt = MenuOption::Vertical();
    modify_opt.on_enter = [&] {
        if (store.all().empty()) return;
        modify_index = modify_selected;
        const Student& s = store.at(modify_index);
        mod_grade_year = s.grade_year;
        mod_birth_date = s.birth_date;
        mod_name       = s.full_name;
        mod_gender     = (s.gender == "Female") ? 1 : 0;
        for (size_t i = 0; i < SUBJECTS.size(); ++i)
            mod_grades[i] = (i < s.grades.size()) ? fmt(s.grades[i]) : "0.00";
        mod_status.clear();
        modify_ok = true;
        modify_stage = 1;
        mod_grade_year_in->TakeFocus();
    };
    auto modify_menu = Menu(&modify_entries, &modify_selected, modify_opt);
    auto modify_back = makeBackButton();
    auto modify_list_container = Container::Vertical({modify_menu, modify_back});
    auto modify_tab = Container::Tab({modify_list_container, modify_edit_container},
                                     &modify_stage);

    auto modify_page = Renderer(modify_tab, [&] {
        if (modify_stage == 0) {
            Element body = store.all().empty()
                ? text("No students to modify.") | dim
                : modify_menu->Render() | vscroll_indicator | yframe;
            return borderedPanel("Modify Student",
                                 vbox({body | flex, text(""),
                                       text(mod_status)
                                           | color(modify_ok ? Color::Green : Color::Red),
                                       modify_back->Render()}),
                                 "Up/Down: select | Enter: edit | Esc: back");
        }
        Elements body;
        body.push_back(hbox(text(paddedLabel("Grade Year")) | size(WIDTH, EQUAL, 14),
                            mod_grade_year_in->Render() | flex));
        body.push_back(hbox(text(paddedLabel("Birth Date")) | size(WIDTH, EQUAL, 14),
                            mod_birth_date_in->Render() | flex));
        body.push_back(hbox(text(paddedLabel("Full Name")) | size(WIDTH, EQUAL, 14),
                            mod_name_in->Render() | flex));
        body.push_back(hbox(text(paddedLabel("Gender")) | size(WIDTH, EQUAL, 14),
                            mod_gender_in->Render()));
        body.push_back(separator());
        for (size_t i = 0; i < SUBJECTS.size(); ++i)
            body.push_back(hbox(text(paddedLabel(SUBJECTS[i])) | size(WIDTH, EQUAL, 14),
                                mod_grade_inputs[i]->Render() | flex));
        body.push_back(text(""));
        body.push_back(hbox(mod_save->Render(), text(" "), mod_cancel->Render()));
        body.push_back(text(""));
        body.push_back(text(mod_status) | color(modify_ok ? Color::Green : Color::Red));

        return borderedPanel("Edit Student",
                             vbox(std::move(body)) | vscroll_indicator | yframe | flex,
                             "Tab/arrows: move | Esc: back to list");
    });

    // ---------------------------------------------------------- REMOVE ----
    std::vector<std::string> remove_entries;
    int remove_selected = 0;
    std::string remove_status;

    auto refreshRemoveList = [&] {
        remove_entries.clear();
        for (const auto& s : store.all())
            remove_entries.push_back(s.full_name + "  [" + s.gender + "]  (" +
                                     s.grade_year + ")");
        if (remove_selected >= static_cast<int>(remove_entries.size()))
            remove_selected = std::max(0, static_cast<int>(remove_entries.size()) - 1);
    };

    MenuOption remove_opt = MenuOption::Vertical();
    remove_opt.on_enter = [&] {
        if (store.all().empty()) return;
        std::string gone = store.at(static_cast<size_t>(remove_selected)).full_name;
        std::string err;
        if (store.remove(static_cast<size_t>(remove_selected), err))
            remove_status = "Removed " + gone + ".";
        else
            remove_status = "Error: " + err;
        refreshRemoveList();
    };
    auto remove_menu = Menu(&remove_entries, &remove_selected, remove_opt);
    auto remove_back = makeBackButton();
    auto remove_container = Container::Vertical({remove_menu, remove_back});

    auto remove_page = Renderer(remove_container, [&] {
        Element body = store.all().empty()
            ? text("No students to remove.") | dim
            : remove_menu->Render() | vscroll_indicator | yframe;
        return borderedPanel("Remove Student",
                             vbox({body | flex, text(""),
                                   text(remove_status) | color(Color::Yellow),
                                   remove_back->Render()}),
                             "Up/Down: select | Enter: remove | Esc: back");
    });

    // ------------------------------------------------------------ VIEW ----
    auto view_back = makeBackButton();
    auto view_page = Renderer(view_back, [&] {
        return borderedPanel("All Students",
                             vbox({studentTable(store.all())
                                       | vscroll_indicator | yframe | flex,
                                   text(""), view_back->Render()}),
                             "Esc: back");
    });

    // ---------------------------------------------------------- SEARCH ----
    std::string query;
    auto query_in = Input(&query, "type part of a name...");
    auto search_back = makeBackButton();
    auto search_container = Container::Vertical({query_in, search_back});
    auto search_page = Renderer(search_container, [&] {
        std::string q = trim(query);
        Element results;
        if (q.empty()) results = text("Start typing a name to search.") | dim;
        else {
            const Student* found = store.findByName(q);
            if (found) results = vbox({text("Exact match:") | bold, studentTable({*found})});
            else {
                std::vector<Student> similar;
                std::string lq = toLower(q);
                for (const auto& s : store.all())
                    if (toLower(s.full_name).find(lq) != std::string::npos)
                        similar.push_back(s);
                if (similar.empty())
                    results = text("No student named \"" + q + "\".") | dim;
                else
                    results = vbox({text("No exact match. Similar names:") | dim,
                                    studentTable(similar)});
            }
        }
        return borderedPanel("Search by Name",
                             vbox({hbox(text("Search: "), query_in->Render() | flex),
                                   separator(),
                                   results | vscroll_indicator | yframe | flex,
                                   text(""), search_back->Render()}),
                             "Results update as you type | Esc: back");
    });

    // ------------------------------------------------------------ SORT ----
    bool descending = true;
    auto flip_btn = Button("Flip order", [&] { descending = !descending; },
                           ButtonOption::Animated());
    auto sort_back = makeBackButton();
    auto sort_container = Container::Horizontal({flip_btn, sort_back});
    auto sort_page = Renderer(sort_container, [&] {
        auto order = store.indicesSortedByAverageDesc();
        if (!descending) std::reverse(order.begin(), order.end());
        std::vector<Student> sorted;
        for (size_t i : order) sorted.push_back(store.at(i));
        return borderedPanel("Sort by Average",
                             vbox({text(descending ? "Order: highest first"
                                                   : "Order: lowest first") | bold,
                                   separator(),
                                   studentTable(sorted) | vscroll_indicator | yframe | flex,
                                   text(""),
                                   hbox(flip_btn->Render(), text(" "),
                                        sort_back->Render())}),
                             "Enter: flip | Esc: back");
    });

    // ----------------------------------------------------------- STATS ----
    std::vector<std::string> stats_views = {"Summary", "Subject Means", "Distribution"};
    int stats_view = 0;
    auto stats_toggle = Toggle(&stats_views, &stats_view);
    auto stats_back = makeBackButton();
    auto stats_container = Container::Vertical({stats_toggle, stats_back});
    auto stats_page = Renderer(stats_container, [&] {
        Element body;
        if (store.all().empty()) body = text("No data yet. Add some students first.") | dim;
        else if (stats_view == 0) body = summaryStatsView(store.all());
        else if (stats_view == 1) body = subjectMeansChart(store.all());
        else                      body = averageDistributionChart(store.all());

        return borderedPanel("Statistics",
                             vbox({stats_toggle->Render() | hcenter, separator(),
                                   body | flex, text(""), stats_back->Render()}),
                             "Left/Right: switch view | Esc: back");
    });

    // ---------------------------------------------------------- EXPORT ----
    std::string export_path, export_status;
    bool export_ok = true;
    bool exit_after_export = false;
    bool export_as_xlsx = false;

    std::vector<std::string> export_formats = {"CSV (.csv)", "XLSX (.xlsx)"};
    int export_format = 0;

    auto performExport = [&] {
        std::string raw = trim(export_path);
        if (raw.empty()) { export_status = "Path cannot be empty."; export_ok = false; return; }

        std::string resolved = util::resolvePath(raw);
        std::filesystem::path p(resolved);
        std::string ext = toLower(p.extension().string());

        std::string target;
        if (ext == ".csv" || ext == ".xlsx") target = resolved;
        else target = resolved + (export_as_xlsx ? ".xlsx" : ".csv");

        if (!store.exportTo(target, export_status)) { export_ok = false; return; }
        export_ok = true;
        if (exit_after_export) { exit_after_export = false; screen.Exit(); return; }
        page = MENU;
        menu_status = "Exported " + std::to_string(store.size()) +
                      " students to " + store.path();
        menu_status_ok = true;
    };

    InputOption export_opt;
    export_opt.on_enter = performExport;

    auto export_path_in   = Input(&export_path, "path/to/students.xlsx", export_opt);
    auto export_format_in = Toggle(&export_formats, &export_format);
    auto export_btn       = Button("Export", performExport, ButtonOption::Animated());
    auto export_back      = Button("Back", [&] {
        exit_after_export = false;
        page = MENU;
    }, ButtonOption::Animated());
    auto export_discard   = Button("Discard & Exit", [&] {
        exit_after_export = false;
        screen.Exit();
    }, ButtonOption::Animated());

    auto export_container = Container::Vertical({
        export_path_in, export_format_in,
        Container::Horizontal({export_btn, export_back}),
        export_discard,
    });

    auto export_page = Renderer(export_container, [&] {
        Elements buttons = {
            export_btn->Render(), text(" "), export_back->Render(),
        };
        if (exit_after_export) {
            buttons.push_back(text(" "));
            buttons.push_back(export_discard->Render());
        }

        Elements body;
        body.push_back(hbox(text("File: ") | size(WIDTH, EQUAL, 6),
                            export_path_in->Render() | flex));
        body.push_back(hbox(text("Format: ") | size(WIDTH, EQUAL, 8),
                            export_format_in->Render()));
        body.push_back(text(""));
        body.push_back(text(std::to_string(store.size()) +
                            " student(s) will be written.") | dim);
        body.push_back(text("Extension in the path wins; toggle is used "
                            "when the path has no .csv/.xlsx suffix.") | dim);
        if (exit_after_export)
            body.push_back(text("Unsaved changes — export to save before exit.")
                           | color(Color::Yellow));
        body.push_back(text(""));
        body.push_back(hbox(std::move(buttons)));
        body.push_back(text(""));
        body.push_back(paragraph(export_status)
                       | color(export_ok ? Color::Green : Color::Red));

        return borderedPanel(exit_after_export ? "Save before exit" : "Export Students",
                             vbox(std::move(body)),
                             "Enter: export | Esc: back");
    });

    // ---------------------------------------------------------- IMPORT ----
    std::string import_path, import_status;
    bool import_ok = true;

    auto performImport = [&] {
        std::string raw = trim(import_path);
        if (raw.empty()) { import_status = "Path cannot be empty."; import_ok = false; return; }

        std::string resolved = util::resolvePath(raw);
        if (store.open(resolved, import_status)) {
            import_ok = true;
            page = MENU;
            menu_status = "Imported " + std::to_string(store.size()) +
                          " students from " + store.path();
            menu_status_ok = true;
        } else {
            import_ok = false;
        }
    };

    InputOption import_opt;
    import_opt.on_enter = performImport;

    auto import_path_in = Input(&import_path, "path/to/students.csv", import_opt);
    auto import_btn     = Button("Import", performImport, ButtonOption::Animated());
    auto import_back    = makeBackButton();

    auto import_container = Container::Vertical({
        import_path_in,
        Container::Horizontal({import_btn, import_back}),
    });

    auto import_page = Renderer(import_container, [&] {
        Elements body;
        body.push_back(hbox(text("File: ") | size(WIDTH, EQUAL, 6),
                            import_path_in->Render() | flex));
        body.push_back(text(""));
        body.push_back(text("Supported: .csv and .xlsx with the same columns.") | dim);
        body.push_back(text("Importing replaces the current session data.") | dim);
        if (store.dirty())
            body.push_back(text("WARNING: current session has unsaved changes; "
                                "they will be discarded.") | color(Color::Yellow));
        body.push_back(text(""));
        body.push_back(hbox(import_btn->Render(), text(" "), import_back->Render()));
        body.push_back(text(""));
        body.push_back(paragraph(import_status)
                       | color(import_ok ? Color::Green : Color::Red));

        return borderedPanel("Import Students",
                             vbox(std::move(body)),
                             "Enter: import | Esc: back");
    });

    // ------------------------------------------------------------ MENU ----
    auto openExportPage = [&](bool forExit) {
        exit_after_export = forExit;
        export_path   = store.bound() ? store.path() : std::string();
        export_as_xlsx = store.bound() ? pathIsXlsx(store.path()) : false;
        export_format  = export_as_xlsx ? 1 : 0;
        export_status.clear();
        export_ok = true;
        page = EXPORT;
        export_path_in->TakeFocus();
    };

    auto doExit = [&] {
        if (!store.dirty()) { screen.Exit(); return; }
        openExportPage(true);
    };

    std::vector<std::string> menu_items = {
        "Add Student", "Modify Student", "Remove Student",
        "View All", "Search by Name", "Sort by Average",
        "Statistics", "Export", "Import", "Exit"
    };
    int menu_selected = 0;

    MenuOption menu_opt = MenuOption::Vertical();
    menu_opt.on_enter = [&] {
        switch (menu_selected) {
            case 0:
                add_grade_year.clear(); add_birth_date.clear();
                add_name.clear();
                for (auto& g : add_grades) g.clear();
                add_gender = 0; add_status.clear(); add_ok = true;
                page = ADD; add_grade_year_in->TakeFocus();
                break;
            case 1:
                mod_status.clear(); modify_ok = true; modify_stage = 0;
                refreshModifyList(); page = MODIFY; break;
            case 2:
                remove_status.clear(); refreshRemoveList(); page = REMOVE; break;
            case 3: page = VIEW; break;
            case 4:
                query.clear(); page = SEARCH; query_in->TakeFocus(); break;
            case 5: page = SORT; break;
            case 6: stats_view = 0; page = STATS; break;
            case 7: openExportPage(false); break;
            case 8:
                import_path.clear(); import_status.clear(); import_ok = true;
                page = IMPORT; import_path_in->TakeFocus();
                break;
            case 9: doExit(); break;
        }
    };
    auto menu = Menu(&menu_items, &menu_selected, menu_opt);

    auto sessionLine = [&] {
        if (!store.bound()) return std::string("File: (unbound)");
        return std::string("File: ") + store.path() +
               (store.dirty() ? "  [unsaved]" : "  [saved]");
    };

    auto menu_page = Renderer(menu, [&] {
        Color sc = Color::Yellow;
        if (!store.bound()) sc = Color::Red;
        else if (!store.dirty()) sc = Color::Green;
        return borderedPanel("Student Records Manager",
                             vbox({
                                 menu->Render() | hcenter,
                                 text(""),
                                 text("Students loaded: " + std::to_string(store.size()))
                                     | dim | hcenter,
                                 text(sessionLine()) | color(sc) | hcenter,
                                 text(menu_status)
                                     | color(menu_status_ok ? Color::Green : Color::Red)
                                     | hcenter,
                             }),
                             "Up/Down: navigate | Enter: select | Esc: exit");
    });

    auto pages = Container::Tab({menu_page, add_page, modify_page, remove_page,
                                 view_page, search_page, sort_page, stats_page,
                                 export_page, import_page}, &page);

    auto root = CatchEvent(pages, [&](Event e) {
        if (e == Event::Escape) {
            if (page == MENU)   { doExit(); return true; }
            if (page == EXPORT) {
                exit_after_export = false;
                page = MENU;
                return true;
            }
            if (page == MODIFY && modify_stage == 1) {
                modify_stage = 0; mod_status.clear();
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

}