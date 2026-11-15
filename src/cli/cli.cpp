#include "cli.h"

#include "core/store.h"
#include "core/validators.h"
#include "util/path.h"
#include "util/str.h"

#include "please/please.h"

#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using util::fmt;
using util::trim;
using util::toLower;

namespace cli {

namespace {

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string cur;
    bool inQuote = false;
    for (char c : line) {
        if (c == '"') inQuote = !inQuote;
        else if (c == ' ' && !inQuote) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else cur += c;
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

bool parseStudentArgs(const std::vector<std::string>& t, size_t start,
                      Student& out, std::string& err) {
    size_t ng = SUBJECTS.size();
    if (t.size() < start + 2 + 1 + 1 + ng) {
        err = "Not enough arguments (need: <grade_year> <birth_date> "
              "\"Full Name\" <m|f> <" + std::to_string(ng) + " grades>).";
        return false;
    }
    out.grade_year = t[start];
    out.birth_date = t[start + 1];

    size_t name_end   = t.size() - ng - 2;
    size_t gender_idx = t.size() - ng - 1;

    out.full_name.clear();
    for (size_t i = start + 2; i <= name_end; ++i) {
        if (!out.full_name.empty()) out.full_name += ' ';
        out.full_name += t[i];
    }
    out.gender = t[gender_idx];

    out.grades.clear();
    for (size_t i = 0; i < ng; ++i) {
        double d = 0.0;
        if (!util::parseDouble(t[t.size() - ng + i], d)) {
            err = "Invalid grade value: " + t[t.size() - ng + i];
            return false;
        }
        out.grades.push_back(d);
    }
    return true;
}

void printHelp() {
    std::cout <<
        "  add <grade_year> <birth_date> \"Full Name\" m|f <"
            << SUBJECTS.size() << " grades>\n"
        "  modify <idx> <grade_year> <birth_date> \"Full Name\" m|f <"
            << SUBJECTS.size() << " grades>\n"
        "  remove <idx>\n"
        "  list\n"
        "  status\n"
        "  import <path.csv|path.xlsx>     Load a file, replacing current data.\n"
        "  export [<path.csv|path.xlsx>]   Write current data (default: bound file).\n"
        "  logout | exit | quit            Prompt to export if unsaved, then exit.\n";
}

void printList(const std::vector<Student>& list) {
    if (list.empty()) { std::cout << "(empty)\n"; return; }

    std::cout << std::left
              << std::setw(5)  << "#"
              << std::setw(14) << "Grade Year"
              << std::setw(14) << "Birth Date"
              << std::setw(24) << "Full Name"
              << std::setw(10) << "Gender";
    for (const auto& sub : SUBJECTS) std::cout << std::setw(10) << sub;
    std::cout << "Avg\n";
    std::cout << std::string(5 + 14 + 14 + 24 + 10 + 10 * SUBJECTS.size() + 8, '-')
              << "\n";

    for (size_t i = 0; i < list.size(); ++i) {
        const auto& s = list[i];
        std::cout << std::left
                  << std::setw(5)  << (i + 1)
                  << std::setw(14) << s.grade_year
                  << std::setw(14) << s.birth_date
                  << std::setw(24) << s.full_name
                  << std::setw(10) << s.gender;
        for (size_t j = 0; j < SUBJECTS.size(); ++j)
            std::cout << std::setw(10)
                      << (j < s.grades.size() ? fmt(s.grades[j]) : std::string("-"));
        std::cout << fmt(studentAverage(s)) << "\n";
    }
}

}

int runShell(const std::string& initialPath) {
    core::Store store;
    std::string err;

    if (!initialPath.empty()) {
        if (!store.open(initialPath, err)) {
            std::cerr << "Error opening file: " << err << "\n";
            return 1;
        }
        std::cout << "Opened " << store.path()
                  << " (" << store.size() << " students).\n";
    } else {
        std::cout << "Started unbound session. "
                     "Use 'import <path>' or 'export <path>' to bind a file.\n";
    }
    std::cout << "Type 'help' for commands, 'logout' to exit.\n\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        auto t = tokenize(line);
        if (t.empty()) continue;
        const std::string& cmd = t[0];

        if (cmd == "help") {
            printHelp();
        } else if (cmd == "list") {
            printList(store.all());
        } else if (cmd == "add") {
            Student s; std::string e2;
            if (!parseStudentArgs(t, 1, s, e2)) { std::cout << "Error: " << e2 << "\n"; continue; }
            if (!store.add(s, e2)) std::cout << "Error: " << e2 << "\n";
            else                    std::cout << "Added.\n";
        } else if (cmd == "modify") {
            if (t.size() < 2) {
                std::cout << "Usage: modify <idx> <grade_year> <birth_date> "
                             "\"Full Name\" m|f <grades...>\n";
                continue;
            }
            size_t idx = 0;
            try { idx = std::stoul(t[1]); }
            catch (...) { std::cout << "Bad index.\n"; continue; }
            if (idx == 0) { std::cout << "Indices are 1-based.\n"; continue; }
            Student s; std::string e2;
            if (!parseStudentArgs(t, 2, s, e2)) { std::cout << "Error: " << e2 << "\n"; continue; }
            if (!store.modify(idx - 1, s, e2)) std::cout << "Error: " << e2 << "\n";
            else                               std::cout << "Modified.\n";
        } else if (cmd == "remove") {
            if (t.size() < 2) { std::cout << "Usage: remove <idx>\n"; continue; }
            size_t idx = 0;
            try { idx = std::stoul(t[1]); }
            catch (...) { std::cout << "Bad index.\n"; continue; }
            if (idx == 0) { std::cout << "Indices are 1-based.\n"; continue; }
            if (!store.remove(idx - 1, err)) std::cout << "Error: " << err << "\n";
            else                             std::cout << "Removed.\n";
        } else if (cmd == "status") {
            std::cout << "File: " << (store.bound() ? store.path()
                                                    : std::string("(unbound)"))
                      << (store.dirty() ? "  [unsaved]" : "  [saved]") << "\n"
                      << "Students: " << store.size() << "\n";
        } else if (cmd == "import") {
            if (t.size() < 2) { std::cout << "Usage: import <path>\n"; continue; }
            if (store.dirty()) {
                std::cout << "Discard unsaved changes? (y/n): " << std::flush;
                std::string resp;
                if (!std::getline(std::cin, resp) ||
                    toLower(trim(resp)) != "y") {
                    std::cout << "Cancelled.\n";
                    continue;
                }
            }
            if (store.open(t[1], err))
                std::cout << "Imported " << store.size()
                          << " students from " << store.path() << "\n";
            else
                std::cout << "Error: " << err << "\n";
        } else if (cmd == "export") {
            std::string target = (t.size() >= 2) ? t[1] : store.path();
            if (target.empty()) {
                std::cout << "Usage: export <path> (or 'export' if a file is bound)\n";
                continue;
            }
            if (store.exportTo(target, err))
                std::cout << "Exported " << store.size()
                          << " students to " << store.path() << "\n";
            else
                std::cout << "Error: " << err << "\n";
        } else if (cmd == "logout" || cmd == "exit" || cmd == "quit") {
            break;
        } else {
            std::cout << "Unknown command '" << cmd << "'. Type 'help'.\n";
        }

        if (std::rand() % 100 < 25) {
            std::cout << "\n[!] PLEASE\n";
            showPleaseImage();
            std::cout << "[!] Window closed. Continuing...\n\n";
        }
    }

    if (store.dirty()) {
        if (store.bound()) {
            std::cout << "\nExport changes to " << store.path()
                      << "? (Y/n): " << std::flush;
            std::string resp;
            if (!std::getline(std::cin, resp)) return 1;
            std::string r = toLower(trim(resp));
            if (r.empty() || r == "y" || r == "yes") {
                if (!store.exportTo(store.path(), err)) {
                    std::cerr << "Error: " << err << "\n";
                    return 1;
                }
                std::cout << "Exported to " << store.path() << "\n";
            } else {
                std::cout << "Discarded.\n";
            }
        } else {
            std::cout << "\nUnsaved changes. Export to (blank to discard): " << std::flush;
            std::string entered;
            if (!std::getline(std::cin, entered) || trim(entered).empty()) {
                std::cout << "Discarded.\n";
            } else if (store.exportTo(trim(entered), err)) {
                std::cout << "Exported to " << store.path() << "\n";
            } else {
                std::cerr << "Error: " << err << "\n";
                return 1;
            }
        }
    } else {
        std::cout << "\nNothing to export.\n";
    }
    return 0;
}

int runValidate(const std::string& path) {
    namespace fs = std::filesystem;
    std::string resolved = util::resolvePath(path);
    if (!fs::exists(resolved)) {
        std::cerr << "Invalid: file does not exist: " << resolved << "\n";
        return 1;
    }
    core::Store store;
    std::string err;
    if (!store.open(resolved, err)) {
        std::cerr << "Invalid: " << err << "\n";
        return 1;
    }
    std::cout << "OK: " << store.path() << " is a valid student file.\n";
    return 0;
}

int runList(const std::string& path) {
    namespace fs = std::filesystem;
    std::string resolved = util::resolvePath(path);
    if (!fs::exists(resolved)) {
        std::cerr << "File does not exist: " << resolved << "\n";
        return 1;
    }
    core::Store store;
    std::string err;
    if (!store.open(resolved, err)) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }
    printList(store.all());
    return 0;
}

}