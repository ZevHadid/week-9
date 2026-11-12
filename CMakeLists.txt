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

add_executable(app ./student_manager.cpp ./app_core.cpp ./helper_functions.cpp)

target_link_libraries(app PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
)