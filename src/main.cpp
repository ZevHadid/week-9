#include "cli/cli.h"
#include "core/store.h"
#include "gui/gui.h"

#include <ctime>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage() {
    std::cout <<
        "Usage:\n"
        "  app gui   [-open <path.xlsx>]   Launch the GUI.\n"
        "  app shell [-open <path.xlsx>]   Interactive session.\n"
        "  app validate <path.xlsx>\n"
        "  app list     <path.xlsx>\n\n"
        "Session files must be .xlsx produced by this app (or empty files).\n"
        "Without -open, no file is bound until you save.\n";
}

}

int main(int argc, char* argv[]) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) { printUsage(); return 1; }

    const std::string& cmd = args[0];

    auto findFlag = [&](const std::string& flag, std::string& out) -> int {
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == flag) {
                if (i + 1 >= args.size()) return -1;
                out = args[i + 1];
                return 0;
            }
        }
        return 1;
    };

    if (cmd == "gui") {
        std::string openPath;
        if (findFlag("-open", openPath) < 0) {
            std::cerr << "-open requires a path.\n";
            return 1;
        }
        core::Store store;
        if (!openPath.empty()) {
            std::string err;
            if (!store.open(openPath, err)) {
                std::cerr << "Error: " << err << "\n";
                return 1;
            }
        }
        return gui::run(store);
    }

    if (cmd == "shell") {
        std::string openPath;
        if (findFlag("-open", openPath) < 0) {
            std::cerr << "-open requires a path.\n";
            return 1;
        }
        return cli::runShell(openPath);
    }

    if (cmd == "validate") {
        if (args.size() < 2) { printUsage(); return 1; }
        return cli::runValidate(args[1]);
    }

    if (cmd == "list") {
        if (args.size() < 2) { printUsage(); return 1; }
        return cli::runList(args[1]);
    }

    std::cerr << "Unknown command: " << cmd << "\n\n";
    printUsage();
    return 1;
}