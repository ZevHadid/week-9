# Student Activity Record

A C++17 terminal application for managing student records, with both an interactive TUI (built with [FTXUI](https://github.com/ArthurSonzogni/FTXUI)) and a scriptable CLI. Sessions can be bound to a `.csv` or `.xlsx` file so your edits persist between runs.

## Prerequisites

Choose **either** the **Docker** workflow (easiest for Windows—no compiler or library setup needed) or the **Native Build** workflow.

### Option A: Docker (Recommended for Windows)
- **Docker Desktop** (installed and running)
- **Windows Terminal** (recommended for proper TUI rendering)

### Option B: Native Build
- **CMake** (version 3.15 or higher)
- **A C++17 compatible compiler** (e.g., GCC, Clang, or MSVC)
- **Build tool** (e.g., Make, Ninja, or Visual Studio)
- **SDL2** development libraries (`libsdl2-dev`)

## Project Structure

```text
.
├── CMakeLists.txt
├── Dockerfile           # Pre-configured Linux build environment
├── FTXUI-main/          # Bundled FTXUI library dependency
├── student_manager.cpp  # Entry point: GUI + CLI dispatch
├── app_core.cpp         # Session state, CRUD, validation
├── app_core.h
├── helper_functions.cpp # Student struct, CSV/XLSX read/write
└── helper_functions.h
```

## How to Compile & Run

### Option A: Using Docker (Windows / Cross-Platform)

1. **Create a `Dockerfile` in the project root** with the following content:

   ```dockerfile
   FROM gcc:13-bookworm

   RUN apt-get update && apt-get install -y \
       cmake \
       libsdl2-dev \
       && rm -rf /var/lib/apt/lists/*

   WORKDIR /app
   COPY . .

   # Build bundled FTXUI once into the image
   RUN cmake -S FTXUI-main -B /tmp/ftxui-build -DFTXUI_BUILD_EXAMPLES=OFF -DFTXUI_BUILD_DOCS=OFF && \
       cmake --build /tmp/ftxui-build -j$(nproc) && \
       cmake --install /tmp/ftxui-build && \
       rm -rf /tmp/ftxui-build

   ENV SDL_AUDIODRIVER=dummy

   # Compile local code on run and forward any CLI arguments to ./build/app
   ENTRYPOINT ["sh", "-c", "mkdir -p build && g++ -g -std=c++20 *.cpp -o build/app -lftxui-component -lftxui-dom -lftxui-screen -lSDL2 && ./build/app \"$@\"", "--"]
   ```

2. **Build the Docker image (run once):**

   Open **Windows Terminal** (or PowerShell) in the project directory and run:

   ```powershell
   docker build -t student-record .
   ```

3. **Compile and run the application (anytime):**

   Mount your current folder (`${PWD}`) so your `.csv`/`.xlsx` files and code edits persist on your computer, and pass any app arguments at the end:

   ```powershell
   # Launch the interactive TUI
   docker run --rm -it -v "${PWD}:/app" student-record gui students.csv

   # Or run any CLI command
   docker run --rm -it -v "${PWD}:/app" student-record list students.csv
   ```

   *(Note: If using legacy Command Prompt instead of PowerShell, replace `${PWD}` with `%cd%`.)*

---

### Option B: Native Build (CMake)

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

   - **Linux / macOS:**

     ```bash
     ./build/app
     ```

   - **Windows (Command Prompt / PowerShell):**

     ```cmd
     .\build\Debug\app.exe
     ```

     *(Note: If you built in Release mode, look in `.\build\Release\app.exe` or `.\build\app.exe` depending on your compiler.)*

### Compilation shortcut (g++)

```bash
g++ -g -std=c++20 path/to/project/*.cpp -o path/to/project/build/app -lftxui-component -lftxui-dom -lftxui-screen -lSDL2
```

## Usage

```text
app <command> [args...]
```

Run with no arguments to print the command list.

### Commands

| Command | Description |
|---|---|
| `app gui [<file>]` | Launch the interactive TUI. If `<file>` is given, the session is bound to it (created if missing). |
| `app shell <file>` | Interactive shell bound to `<file>`. Creates it if it doesn't exist. |
| `app add <file> "<Name>" <m\|f> <score>` | Append a student, save, and exit. |
| `app modify <file> <idx> "<Name>" <m\|f> <score>` | Update a student (1-based index), save, and exit. |
| `app remove <file> <idx>` | Delete a student, save, and exit. |
| `app list <file>` | Print all students in the file. |
| `app validate <file>` | Check that a file is a well-formed student record without modifying it. |
| `app import <file> <source>` | Append rows from `<source>` (CSV or XLSX) into `<file>`. |
| `app export <file> <dest>` | Copy `<file>` to `<dest>`. Format chosen by extension. |

**Notes on arguments**
- Names with spaces work **with or without** quotes — `"John Smith"` and `John Smith` are equivalent.
- Gender accepts `m`/`f` (short) or `Male`/`Female` (long).
- Scores must be a number between **0 and 100**.
- Indices are **1-based** and match the `list` output.

### File formats

- **CSV** — UTF-8 with BOM, `\r\n` line endings, header `Name,Gender,Score`. Excel-safe (formula-injection guard on names).
- **XLSX** — minimal single-sheet workbook. Files written by this app round-trip cleanly. Excel-saved `.xlsx` files use DEFLATE compression that this build can't decode — if you need to import one, **save it as CSV first**.

Columns in either format: `Name`, `Gender`, `Score`. The header row is optional when importing.

### Interactive shell

```bash
./build/app shell students.csv
```

```text
> help
  add "First Last" m|f <score>      Append a student
  modify <idx> "First Last" m|f <score>
  remove <idx>
  list
  status
  save
  logout                            Save and exit

> add "John Smith" m 92
Added.
> list
#    Name                    Gender    Score
--------------------------------------------------
1    John Smith              Male      92.00
> modify 1 John Smith m 95
Modified.
> logout

Saved to students.csv
```

### GUI

```bash
./build/app gui students.csv
```

The status line at the bottom of the menu shows the bound file (`[saved]` / `[unsaved]`). Changes are auto-saved on exit if the session is bound to a file. Launching without a file still works, but the **Save** menu item will prompt you to relaunch with a file path.

GUI features: add / modify / remove, view all, name search with fuzzy fallback, sort by score, statistics (summary table, per-student bars, histogram), and import / export.

### Import behaviour

The **Import** page and `app import` offer two modes when merging into a bound file:

- **Replace all** — discard the current students and load only the imported rows.
- **Append** — keep existing students and add imported rows to the end.

Validation runs *before* any in-memory state changes, so a malformed file never wipes your data.

## Exit Codes

| Code | Meaning |
|---|---|
| `0` | Success |
| `1` | Bad arguments, file error, or validation failure |
