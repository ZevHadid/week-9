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
    return 0;
}