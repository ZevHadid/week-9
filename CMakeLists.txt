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

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-2.30.11
)

# Disable building tests and extra targets for SDL to speed up configuration
set(SDL_TEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui SDL2)

add_executable(app ./student_manager.cpp ./app_core.cpp ./helper_functions.cpp ./surprise.cpp)

target_link_libraries(app PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    # Link the SDL2 target, but explicitly exclude SDL2main so it uses your standard int main()
    SDL2::SDL2
)