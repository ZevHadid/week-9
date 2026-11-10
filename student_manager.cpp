// Remember -lncurses flag when compiling in Linux/MacOS

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <cmath> 
#include <iomanip>
#include "helper_functions.h"

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <ncurses.h>
#endif

// Helper function to clear screen on Windows
void clearScreenWin() {
    #ifdef _WIN32
        system("cls");
    #endif
}

void addStudent() {
    std::string name;
    double score;

    std::cout << "Enter student name: ";
    std::getline(std::cin, name);
    std::cout << "Enter student score: ";
    std::cin >> score;
    std::cin.ignore();

    Student newStudent{name, score};
    students.push_back(newStudent);
    scores.push_back(score);

    count = static_cast<double>(scores.size());
    mean = std::accumulate(scores.begin(), scores.end(), 0.0) / scores.size();

    variance_sum = 0.0; 
    for(double x : scores) {
        variance_sum += std::pow(x - mean, 2);
    }
    
    if (scores.size() > 1) {
        std_dev = std::sqrt(variance_sum / (scores.size() - 1));
    } else {
        std_dev = 0.0; 
    }

    min_score = *std::min_element(scores.begin(), scores.end());
    max_score = *std::max_element(scores.begin(), scores.end());

    auto p50_it = scores.begin() + static_cast<size_t>(0.50 * (scores.size() - 1));
    std::nth_element(scores.begin(), p50_it, scores.end());
    p50 = *p50_it;

    std::cout << "Student added: " << newStudent.name << " with score " << newStudent.score << "\n";
}

void displayStatics() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Statistics:\n";
    std::cout << "Count: " << count << "\n";
    std::cout << "Mean: " << mean << "\n";
    std::cout << "Standard Deviation: " << std_dev << "\n";
    std::cout << "Minimum Score: " << min_score << "\n";
    std::cout << "Maximum Score: " << max_score << "\n";
    std::cout << "50th Percentile (Median): " << p50 << "\n";
    std::cin.ignore();
    
}

// Reusable helper function to pause until the user types "Exit"
void waitForExit() {
    std::string exitStr;
    while (true) {
        std::cout << "\nType 'Exit' to return to menu: ";
        std::getline(std::cin >> std::ws, exitStr);

        for (char& c : exitStr) {
            c = std::toupper(static_cast<unsigned char>(c));
        }

        if (exitStr == "EXIT") {
            break;
        }
        
        std::cout << "Invalid input. Please type 'Exit'.\n";
    }
}

void displayAllStudents() {
    if (students.empty()) {
        std::cout << "No students to display.\n";
        waitForExit(); // Prompts user to type 'Exit' before returning
        return;
    }

    std::cout << std::left << std::setw(15) << "Name" << std::setw(10) << "Score" << std::endl;
    std::cout << "--------------------\n";
    for (const auto& student : students) {
        std::cout << std::left << std::setw(15) << student.name << std::setw(10) << student.score << std::endl;
    }

    waitForExit(); // Prompts user to type 'Exit' before returning
}

int runMenu(const std::vector<std::string>& options) {
    int selected = 0;
    int numOptions = options.size();

#ifdef _WIN32
    // === WINDOWS IMPLEMENTATION (<conio.h>) ===
    while (true) {
        clearScreenWin();
        std::cout << "          ----- Student Records Manager ------\n\n";
        std::cout << "[ Use UP/DOWN arrows to navigate | press ENTER to confirm]\n\n";

        for (int i = 0; i < numOptions; ++i) {
            if (i == selected) {
                std::cout << " > [" << options[i] << "]\n";
            } else {
                std::cout << "    " << options[i] << "\n";
            }
        }

        int key = _getch();
        if (key == 224 || key == 0) { // Arrow key prefix
            key = _getch(); // Get actual arrow code
            if (key == 72) { // Up arrow
                selected = (selected - 1 + numOptions) % numOptions;
            } else if (key == 80) { // Down arrow
                selected = (selected + 1) % numOptions;
            }
        } else if (key == 13) { // Enter key
            return selected;
        }
    }

#else
    // === LINUX / MACOS IMPLEMENTATION (<ncurses.h>) ===
    initscr();            // Start ncurses mode
    noecho();             // Don't echo typed characters
    cbreak();             // Disable line buffering
    keypad(stdscr, TRUE); // Enable arrow keys
    curs_set(0);          // Hide cursor

    while (true) {
        clear();
        printw("Use UP/DOWN arrows and press ENTER:\n\n");

        for (int i = 0; i < numOptions; ++i) {
            if (i == selected) {
                attron(A_REVERSE); // Highlight selected item
                printw(" > %s \n", options[i].c_str());
                attroff(A_REVERSE);
            } else {
                printw("   %s \n", options[i].c_str());
            }
        }
        refresh();

        int key = getch();
        if (key == KEY_UP) {
            selected = (selected - 1 + numOptions) % numOptions;
        } else if (key == KEY_DOWN) {
            selected = (selected + 1) % numOptions;
        } else if (key == 10 || key == KEY_ENTER) { // Enter key
            endwin(); // Restore normal terminal before returning
            return selected;
        }
    }
#endif
}

int main() {
    std::vector<std::string> menuItems = {
        "Add Student",
        "Remove Student",
        "View All",
        "Search by Name",
        "Sort by Score",
        "Statistics",
        "Exit"
    };

    int choice = runMenu(menuItems);
    std::cout << "\nYou selected: " << menuItems[choice] << "\n";

    bool running = true;
    while (running) {
        int choice = runMenu(menuItems);
        std::cout << "\nYou selected: " << menuItems[choice] << "\n";

        switch (choice) {
            case 0:
                addStudent();
                break;
            case 1:
                std::cout << "Remove Student functionality not implemented yet.\n";
                break;
            case 2:
                displayAllStudents();
                break;
            case 3:
                std::cout << "Search by Name functionality not implemented yet.\n";
                break;
            case 4:
                std::cout << "Sort by Score functionality not implemented yet.\n";
                break;
            case 5:
                displayStatics();
                break;
            case 6:
                std::cout << "Exiting the program. Goodbye!\n";
                running = false;
                break;
            default:
                std::cout << "Invalid choice.\n";
        }
    }

    return 0;
}