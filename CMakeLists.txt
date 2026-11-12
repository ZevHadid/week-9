cmake_minimum_required(VERSION 3.15)
project(STUDENT_MANAGER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# 1. Fetch FTXUI
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG        v7.0.3
)
FetchContent_MakeAvailable(ftxui)

# 2. Fetch SDL2 so it builds automatically on Windows without manual setup
FetchContent_Declare(
  sdl2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG        release-2.30.2
)
FetchContent_MakeAvailable(sdl2)

add_executable(app ./student_manager.cpp ./app_core.cpp ./helper_functions.cpp ./surprise.cpp)

target_link_libraries(app PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    SDL2::SDL2
    SDL2::SDL2main
)