# Student Activity Record

A C++17 terminal application for managing student records, with both an interactive TUI (built with [FTXUI](https://github.com/ArthurSonzogni/FTXUI)) and a scriptable CLI. Sessions can be bound to a `.csv` or `.xlsx` file.

## Prerequisites

Choose **either** the **Docker** workflow (easiest for Windows, no compiler or library setup) or the **Native Build** workflow.

### Option A: Docker (Recommended for Windows)

- **Docker Desktop** (installed and running)
- **Windows Terminal** (recommended for proper TUI rendering)

### Option B: Native Build

- **CMake** (3.15+) **or** just `g++` if FTXUI/SDL2 are already installed
- **Git** (CMake uses it to download dependencies)
- **A C++17-compatible compiler** (GCC/MinGW, Clang, or MSVC)
- **Build tool** (Make, Ninja, or Visual Studio) — only for the CMake path
- **An internet connection on the first configure.** `CMakeLists.txt` downloads and builds **FTXUI** (v7.0.3) and **SDL2** (release-2.30.11) automatically via `FetchContent`.

## Project Structure

```text
.
├── CMakeLists.txt
├── .gitignore
├── Dockerfile
└── src/
    ├── main.cpp                      # Entry point: dispatch gui / shell / validate / list
    ├── cli/
    │   ├── cli.h
    │   └── cli.cpp                   # Interactive shell, validate, list
    ├── core/
    │   ├── student.h / student.cpp   # Student struct + SUBJECTS + average helper
    │   ├── validators.h / .cpp       # Field validation, gender normalization
    │   └── store.h / store.cpp       # In-memory state, CRUD, open/export
    ├── gui/
    │   ├── gui.h / gui.cpp           # FTXUI pages, menu, save-before-exit flow
    │   ├── widgets.h / .cpp          # Student table, gender colors, bordered panel
    │   └── charts.h / .cpp           # Summary table, subject means, histogram
    ├── io/
    │   ├── csv_read.h / .cpp         # CSV parser (RFC 4180 quoted fields)
    │   ├── csv_write.h / .cpp        # CSV writer (UTF-8 BOM, CRLF, formula guard)
    │   ├── xlsx_read.h / .cpp        # Minimal .xlsx reader (inlineStr + numeric)
    │   ├── xlsx_write.h / .cpp       # Minimal .xlsx writer
    │   ├── read.h / .cpp             # readFile() — dispatches by extension
    │   ├── write.h / .cpp            # writeFile() — dispatches by extension
    │   ├── zip.h / .cpp              # Stored-only zip reader/writer
    │   └── xml.h / .cpp              # Cell / column helpers, XML escape
    └── util/
        ├── str.h / str.cpp           # trim, toLower, fmt, parseDouble
        └── path.h / path.cpp         # cleanPath, expandTilde, resolvePath
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
| `ENTRYPOINT` | On **every run** it creates `build/`, compiles `src/*.cpp src/*/*.cpp` with `g++ -g -std=c++20 -Isrc`, links `-lftxui-component -lftxui-dom -lftxui-screen -lSDL2`, and then runs `./build/app` with the arguments you passed. |

> **If your Dockerfile still globs a flat `*.cpp`**, update it to `src/*.cpp src/*/*.cpp` and add `-Isrc` — otherwise `src/cli/cli.cpp` and friends won't be compiled.

Because the entrypoint compiles on each run, code edits on your computer are picked up automatically — no image rebuild needed. The only cost is a few seconds per launch.

#### Steps

1. **Build the Docker image (run once, or when the Dockerfile changes):**

   ```powershell
   docker build -t student-record .
   ```

2. **Compile and run the application (anytime):**

   ```powershell
   docker run --rm -it -v "${PWD}:/app" student-record gui -open students.xlsx
   docker run --rm -it -v "${PWD}:/app" student-record list students.xlsx
   ```

   *(If using legacy Command Prompt, replace `${PWD}` with `%cd%`.)*

#### Docker notes and limitations

- **The please window can't open in Docker.** No display, so `SDL_Init` fails with "No available video device". The app prints that error and carries on. Use a native build to see the window.
- **The output is a Linux binary** at `build/app` inside your project folder. It will not run on Windows and shares `build/` with a native CMake build — delete `build/` when switching between the two.
- **Use `-it`.** The TUI needs an interactive terminal.
- **The container clones FTXUI's default branch**, while the CMake build pins **v7.0.3**. If the two ever behave differently, that's the first thing to check.

---

### Option B: Native Build (CMake)

1. **From the project root:**

   ```bash
   cmake -B build
   cmake --build build
   ./build/app gui -open students.xlsx
   ```

   The first configure downloads FTXUI and SDL2 (a minute or two). SDL is built **statically** on purpose, so there's no `SDL2.dll` to lose. On MinGW the compiler runtime is linked statically too.

   > If you ever change SDL or link options, delete `build/` and reconfigure — old values are cached.

### Compilation shortcut (g++)

If FTXUI and SDL2 are already installed system-wide (which is what the Docker image does), you can skip CMake entirely:

```bash
g++ -g -std=c++20 -Isrc src/*.cpp src/*/*.cpp -o build/app \
  -lftxui-component -lftxui-dom -lftxui-screen -lSDL2
```

`-Isrc` is what resolves `#include "core/store.h"`, `#include "please/please.h"`, and friends. Without it, every cross-file include fails.

### Troubleshooting

**`app.exe` closes immediately with no output (Windows).** This usually means a required DLL was missing and the program died before `main()`. Check the exit code right after the failed run:

```powershell
$LASTEXITCODE
```

- `-1073741515` (`0xC0000135`): DLL missing. Rebuild with the current `CMakeLists.txt` (static SDL2), or copy `SDL2.dll` next to `app.exe`.
- `-1073741701` (`0xC000007B`): 32-bit / 64-bit mismatch between the exe and a DLL.

**The please window never appears.** It only runs in `shell` mode and only has a **25% chance** after each command. On machines with no display (SSH, containers) or no GPU acceleration, SDL may fail and print an error instead.

**`fatal error: 'please/please.h' file not found` (or `'surprise.h'` not found).** Two related things to check:
- Build with `-Isrc` (the g++ command above already does this).
- If `src/cli/cli.cpp` still has `#include "surprise.h"`, change it to `#include "please/please.h"` — the file was renamed to `please/`.
- If your `CMakeLists.txt` still lists `src/surprise.cpp`, change it to `src/please/please.cpp`.

## Usage

```text
app <command> [args...]
```

Run with no arguments to print the command list.

### Commands

| Command | Description |
|---|---|
| `app gui [-open <file>]` | Launch the interactive TUI. Without `-open` the session starts unbound; you can bind it with **Import** or **Export**. |
| `app shell [-open <file>]` | Interactive shell. Without `-open` the session starts unbound; use `import`/`export` to bind. |
| `app list <file>` | Print all students in `<file>`. |
| `app validate <file>` | Check that `<file>` is a well-formed student record without modifying it. |

**`-open` is the only way to bind a file at launch.** If the path doesn't exist yet, the session opens empty and binds to that path — the file is written on first export.

### Schema

| Column | Notes |
|---|---|
| Grade Year | Free text (e.g. `Freshman`). |
| Birth Date | `YYYY-MM-DD`. |
| Full Name | No digits. Must be unique per gender. |
| Gender | `m` / `f` (short) or `Male` / `Female` (long). |
| Math, Science, English, History | Each a number 0–100. |

A student with the same `Full Name` and `Gender` as an existing one is rejected as a duplicate. Indices (in the shell and `list`) are **1-based**.

### File formats

Both `.csv` and `.xlsx` are round-trip capable:

- **CSV**: UTF-8 with BOM, `\r\n` line endings, header row required on import, formula-injection guard on names.
- **XLSX**: minimal single-sheet workbook with the same columns. Files written by this app round-trip cleanly. Excel-saved `.xlsx` uses DEFLATE compression this build can't decode, so **save as CSV first** if you need to import an Excel-authored file.

Columns in either format, in order: `Grade Year`, `Birth Date`, `Full Name`, `Gender`, `Math`, `Science`, `English`, `History`.

### Interactive shell

```bash
./build/app shell -open students.xlsx
```

```text
> help
  add <grade_year> <birth_date> "Full Name" m|f <4 grades>
  modify <idx> <grade_year> <birth_date> "Full Name" m|f <4 grades>
  remove <idx>
  list
  status
  import <path.csv|path.xlsx>     Load a file, replacing current data.
  export [<path.csv|path.xlsx>]   Write current data (default: bound file).
  logout | exit | quit            Prompt to export if unsaved, then exit.

> add Freshman 2008-04-12 "John Smith" m 92 88 95 80
Added.
> list
#    Grade Year    Birth Date    Full Name                Gender    Math      Science   English   History   Avg
----------------------------------------------------------------------------------------------------------
1    Freshman      2008-04-12    John Smith               Male      92.00     88.00     95.00     80.00     88.75
> modify 1 Freshman 2008-04-12 John Smith m 95 90 96 82
Modified.
> logout

Export changes to students.xlsx? (Y/n): y
Exported to students.xlsx
```
### GUI

```bash
./build/app gui -open students.xlsx
```

The status line at the bottom of the menu shows the bound file (`[saved]` / `[unsaved]`). Menu items:

- **Add / Modify / Remove** — full-schema forms (grade year, birth date, name, gender, per-subject grades).
- **View All** — full table.
- **Search by Name** — exact match, with a "similar names" fallback.
- **Sort by Average** — flippable order.
- **Statistics** — Summary, Subject Means, Distribution.
- **Export** — writes the current data to a `.csv` or `.xlsx` path. If the path matches the bound file, it overwrites (an update). If it differs, the session **rebinds** to the new path (a copy). The extension on the path wins; the format toggle is only used when no extension is given.
- **Import** — reads `.csv` or `.xlsx` and **replaces** the current data. Warns if there are unsaved changes first.
- **Exit** — if the session is dirty, routes you to the Export page. Export → exit. Cancel → back to menu. **Discard & Exit** → leave without writing.

There is **no Save / Save As**. Persistence is only through Import and Export.

## Exit Codes

| Code | Meaning |
|---|---|
| `0` | Success |
| `1` | Bad arguments, file error, or validation failure |
