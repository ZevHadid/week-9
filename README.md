# Student Activity Record

A C++17 terminal application for managing student records, with both an interactive TUI (built with [FTXUI](https://github.com/ArthurSonzogni/FTXUI)) and a scriptable CLI. Sessions can be bound to a `.csv` or `.xlsx` file so your edits persist between runs.

The interactive `shell` mode also has a small SDL2 "surprise" feature (see [Surprise window](#surprise-window-sdl2)).

## Prerequisites

Choose **either** the **Docker** workflow (easiest for Windows, with no compiler or library setup) or the **Native Build** workflow.

### Option A: Docker (Recommended for Windows)

- **Docker Desktop** (installed and running)
- **Windows Terminal** (recommended for proper TUI rendering)

### Option B: Native Build

- **CMake** (version 3.15 or higher)
- **Git** (CMake uses it to download dependencies)
- **A C++17 compatible compiler** (e.g., GCC/MinGW, Clang, or MSVC)
- **Build tool** (e.g., Make, Ninja, or Visual Studio)
- **An internet connection on the first configure.** `CMakeLists.txt` downloads and builds **FTXUI** (v7.0.3) and **SDL2** (release-2.30.11) automatically with `FetchContent`, so you do **not** need to install them yourself.

## Project Structure

```text
.
├── CMakeLists.txt
├── Dockerfile           # Pre-configured Linux build environment
├── student_manager.cpp  # Entry point: GUI + CLI dispatch
├── app_core.cpp         # Session state, CRUD, validation
├── app_core.h
├── helper_functions.cpp # Student struct, CSV/XLSX read/write
├── helper_functions.h
├── surprise.cpp         # SDL2 window that shows the embedded image
├── surprise.h
└── surprise_image.h     # Embedded BMP image data (byte array)
```

## How to Compile & Run

### Option A: Using Docker (Windows / Cross-Platform)

#### What the Dockerfile does

The image is based on `gcc:13-bookworm` (Debian 12 with GCC 13) and is set up as follows:

| Step | What it does |
|---|---|
| `apt-get install` | Installs `cmake`, `git` and `libsdl2-dev` (SDL2 headers and library). |
| FTXUI build | Clones FTXUI from GitHub (shallow clone), builds it with examples and docs turned off, installs it system-wide, then deletes the sources. No local FTXUI folder is needed. |
| `SDL_AUDIODRIVER=dummy` | Stops SDL from complaining about missing audio devices inside the container. |
| `WORKDIR /app` | Your project folder is mounted here at run time. |
| `ENTRYPOINT` | On **every run** it creates `build/`, compiles all `*.cpp` files with `g++ -g -std=c++20`, links `-lftxui-component -lftxui-dom -lftxui-screen -lSDL2`, and then runs `./build/app` with the arguments you passed. |

Because the entrypoint compiles on each run, code edits on your computer are picked up automatically and there is no need to rebuild the image. The only cost is a few seconds of compile time on every launch.

#### Steps

1. **Build the Docker image (run once, or when the Dockerfile changes):**

   Open **Windows Terminal** (or PowerShell) in the project directory and run:

   ```powershell
   docker build -t student-record .
   ```

2. **Compile and run the application (anytime):**

   Mount your current folder (`${PWD}`) so your `.csv`/`.xlsx` files and code edits persist on your computer, and pass any app arguments at the end:

   ```powershell
   # Launch the interactive TUI
   docker run --rm -it -v "${PWD}:/app" student-record gui students.csv

   # Or run any CLI command
   docker run --rm -it -v "${PWD}:/app" student-record list students.csv
   ```

   *(Note: If using legacy Command Prompt instead of PowerShell, replace `${PWD}` with `%cd%`.)*

#### Docker notes and limitations

- **The surprise window can't open in Docker.** The container has no display, so `SDL_Init` fails with an error such as "No available video device". The app prints that error and carries on normally. Everything else works the same. To see the window, use a native build.
- **The output is a Linux binary.** It is written to `build/app` inside your project folder. It will not run on Windows, and it shares the `build/` folder with a native CMake build (see Option B). If that causes confusion, delete `build/` when you switch between the two.
- **Use `-it`.** The TUI needs an interactive terminal.
- **The container uses the latest FTXUI.** The Docker image clones FTXUI's default branch, while the CMake build pins **v7.0.3**. If the two ever behave differently, that is the first thing to check.

---

### Option B: Native Build (CMake)

1. **Clone or navigate to the project directory:**

   ```bash
   cd path/to/your/project
   ```

2. **Configure CMake:**

   ```bash
   cmake -B build
   ```

   The first run downloads FTXUI and SDL2, so it takes a while. SDL2 is built as a **static** library, so there is no `SDL2.dll` to ship or lose. On MinGW the compiler runtime (`libstdc++`, `libgcc`, `libwinpthread`) is linked statically too.

   > If you ever change SDL or link options, delete the `build/` folder and re-run this step, because the old values are cached.

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
     .\build\app.exe
     ```

     *(Note: Multi-config generators such as Visual Studio put the exe in a configuration folder instead, e.g. `.\build\Debug\app.exe` or `.\build\Release\app.exe`.)*

### Compilation shortcut (g++)

Requires FTXUI and SDL2 to already be installed on your system (this is what the Docker image does):

```bash
g++ -g -std=c++20 path/to/project/*.cpp -o path/to/project/build/app -lftxui-component -lftxui-dom -lftxui-screen -lSDL2
```

### Troubleshooting

**`app.exe` closes immediately with no output (Windows).** This usually means a required DLL could not be found, so the program dies before `main()` runs. Check the exit code right after the failed run:

```powershell
$LASTEXITCODE
```

- `-1073741515` (`0xC0000135`): a DLL is missing. Rebuild with the current `CMakeLists.txt` (static SDL2), or copy `SDL2.dll` (and the MinGW runtime DLLs, if you use MinGW without static linking) next to `app.exe`.
- `-1073741701` (`0xC000007B`): 32-bit / 64-bit mismatch between the exe and a DLL.

**The surprise window never appears.** It only runs in `shell` mode and only has a 30% chance after each command. On machines with no display (SSH, containers) or no GPU acceleration, SDL may fail and print an error instead.

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
- Names with spaces work **with or without** quotes: `"John Smith"` and `John Smith` are equivalent.
- Names cannot contain digits.
- Gender accepts `m`/`f` (short) or `Male`/`Female` (long).
- Scores must be a number between **0 and 100**.
- A student with the same name and gender as an existing one is rejected as a duplicate.
- Indices are **1-based** and match the `list` output.

### File formats

- **CSV**: UTF-8 with BOM, `\r\n` line endings, header `Name,Gender,Score`. Excel-safe (formula-injection guard on names).
- **XLSX**: minimal single-sheet workbook. Files written by this app round-trip cleanly. Excel-saved `.xlsx` files use DEFLATE compression that this build can't decode, so if you need to import one, **save it as CSV first**.

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

#### Surprise window (SDL2)

After each shell command there is a **30% chance** that a 600×600 SDL2 window opens showing an image embedded in the program (`surprise_image.h`). Close the window to continue. If SDL can't open a window (for example in Docker), the app prints an SDL error and the shell carries on.

### GUI

```bash
./build/app gui students.csv
```

The status line at the bottom of the menu shows the bound file (`[saved]` / `[unsaved]`). Changes are auto-saved on exit if the session is bound to a file. Launching without a file still works, but the **Save** menu item will prompt you to relaunch with a file path.

GUI features: add / modify / remove, view all, name search (exact match, with a "similar names" fallback), sort by score, statistics (summary table, per-student bars, histogram), and import / export.

### Import behaviour

The **Import** page offers two modes when merging into a bound file:

- **Replace all**: discard the current students and load only the imported rows.
- **Append**: keep existing students and add imported rows to the end.

The `app import` command always appends.

Rows are parsed and validated before the current data is replaced, so a malformed file never wipes your data.

## Exit Codes

| Code | Meaning |
|---|---|
| `0` | Success |
| `1` | Bad arguments, file error, or validation failure |