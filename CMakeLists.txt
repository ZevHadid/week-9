cmake_minimum_required(VERSION 3.15)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG        v7.0.3
)

FetchContent_MakeAvailable(ftxui)

# Find SDL2
find_package(SDL2 REQUIRED)

add_executable(app ./student_manager.cpp ./app_core.cpp ./helper_functions.cpp ./surprise.cpp)

target_link_libraries(app PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    SDL2::SDL2
)

# SDL2main is required on Windows, but will cause linker errors on Linux/macOS
if(WIN32)
    target_link_libraries(app PRIVATE SDL2::SDL2main)
endif()