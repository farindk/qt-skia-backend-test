cmake_minimum_required(VERSION 3.10)
project(qtskia)

option(IM_DESKTOP_ENABLE_WINDOWS_CONSOLE "Enable the console window with logging output" OFF)

set(IM_SYSTEM "Linux" CACHE STRING "System (Linux/Windows)")

set(IM_FONTS_DIR_DEVELOP "${PROJECT_SOURCE_DIR}/fonts" CACHE INTERNAL "Directory where fonts can be found (during development)" FORCE)

add_subdirectory(sources)
