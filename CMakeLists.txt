cmake_minimum_required(VERSION 3.15)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(FTXUI-main)

add_executable(app ./test.cpp)
target_link_libraries(app PRIVATE ftxui::ftxui)