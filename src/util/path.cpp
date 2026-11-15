#include "path.h"
#include "str.h"

#include <cstdlib>

namespace util {

std::string cleanPath(std::string s) {
    s = trim(s);
    if (s.size() >= 2 && ((s.front() == '"'  && s.back() == '"') ||
                          (s.front() == '\'' && s.back() == '\'')))
        s = trim(s.substr(1, s.size() - 2));
    return s;
}

std::string expandTilde(std::string s) {
    if (!s.empty() && s[0] == '~') {
        if (const char* home = std::getenv("HOME"))
            s = std::string(home) + s.substr(1);
    }
    return s;
}

std::string resolvePath(const std::string& raw) {
    return expandTilde(cleanPath(raw));
}

}