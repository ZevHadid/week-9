cmake_minimum_required(VERSION 3.15)
project(student_records LANGUAGES CXX)

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

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui SDL2)

add_executable(app
  src/main.cpp
  src/please/please.cpp
  src/cli/cli.cpp
  src/gui/gui.cpp
  src/gui/widgets.cpp
  src/gui/charts.cpp
  src/core/student.cpp
  src/core/validators.cpp
  src/core/store.cpp
  src/io/csv_read.cpp
  src/io/csv_write.cpp
  src/io/read.cpp
  src/io/write.cpp
  src/io/xlsx_read.cpp
  src/io/xlsx_write.cpp
  src/io/zip.cpp
  src/io/xml.cpp
  src/util/str.cpp
  src/util/path.cpp
)

target_include_directories(app PRIVATE src)

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

if(MINGW)
  target_link_options(app PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(TARGET SDL2::SDL2 AND NOT APP_SDL_TARGET STREQUAL "SDL2::SDL2-static")
  add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:app>"
  )
endif()