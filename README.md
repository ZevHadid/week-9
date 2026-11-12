# Student Activity Record

A C++17 terminal application built with [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

## Prerequisites

Before building the project, make sure you have the following installed:

* **CMake** (version 3.15 or higher)
* **A C++17 compatible compiler** (e.g., GCC, Clang, or MSVC)
* **Build tool** (e.g., Make, Ninja, or Visual Studio)

## Project Structure

```text
.
├── CMakeLists.txt
├── FTXUI-main/          # Bundled FTXUI library dependency
├── student_manager.cpp  # Main application source
└── helper_functions.cpp # Helper utilities
```

## How to Compile & Run

1. **Clone or navigate to the project directory:**
   ```bash
   cd path/to/your/project
   ```

2. **Create a build directory and configure CMake:**
   ```bash
   cmake -B build
   ```

3. **Compile the project:**
   ```bash
   cmake --build build
   ```

4. **Run the executable:**
   * **Linux / macOS:**
     ```bash
     ./build/app
     ```
   * **Windows (Command Prompt / PowerShell):**
     ```cmd
     .\build\Debug\app.exe
     ```
     *(Note: If you built in Release mode, look in `.\build\Release\app.exe` or `.\build\app.exe` depending on your compiler.)*

### lil compilation shortcut here if you use g++
```bash
g++ -g -std=c++20 path/to/your/project/*.cpp -o path/to/your/project/build/app -lftxui-component -lftxui-dom -lftxui-screen
```
