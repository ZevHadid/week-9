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

# Build SDL as a static library so there is no SDL2.dll to go missing.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui SDL2)

add_executable(app
  student_manager.cpp
  app_core.cpp
  helper_functions.cpp
  surprise.cpp
)

# Static-only SDL exposes SDL2::SDL2-static; fall back to SDL2::SDL2 if not.
# SDL2main is deliberately not linked, since the app uses a standard int main().
if(TARGET SDL2::SDL2-static)
  set(APP_SDL_TARGET SDL2::SDL2-static)
else()
  set(APP_SDL_TARGET SDL2::SDL2)
endif()

target_link_libraries(app PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  ${APP_SDL_TARGET}
)

# MinGW: bake the compiler runtime into the exe (no libstdc++-6.dll etc.).
if(MINGW)
  target_link_options(app PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# Safety net: if SDL ever ends up shared, copy the DLL next to the exe.
if(TARGET SDL2::SDL2 AND NOT APP_SDL_TARGET STREQUAL "SDL2::SDL2-static")
  add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:app>"
  )
endif()