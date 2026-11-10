// Remember -lncurses flag when compiling in Linux/MacOS

#include <iostream>
#include <vector>
#include <string>

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

struct Student {
    std::string name;
    int score;
};

std::vector<Student> students;

void addStudent() {
    std::string name;
    int score;

    std::cout << "Enter student name: ";
    std::getline(std::cin, name);
    std::cout << "Enter student score: ";
    std::cin >> score;
    std::cin.ignore();

    Student newStudent{name, score};
    students.push_back(newStudent);

    std::cout << "Student added: " << newStudent.name << " with score " << newStudent.score << "\n";
}

void displayStatics() {
    std::cout << "Displaying statistics...\n";
    
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
                std::cout << "View All functionality not implemented yet.\n";
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