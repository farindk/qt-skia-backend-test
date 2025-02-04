
cmake_minimum_required(VERSION 3.10)

find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 20)

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
# Resource compiler
set(CMAKE_AUTORCC ON)

if (IM_DESKTOP_ENABLE_WINDOWS_CONSOLE)
    add_executable(qtskia)
else ()
    add_executable(qtskia WIN32)
endif ()

target_sources(qtskia PRIVATE
        main.cpp
        main/MainWindow.h
        main/MainWindow.cpp
        resources/resources.qrc
        drawing/Drawing.h
        drawing/Drawing.cc
        drawing/DrawingWidget_Skia_GL.h
        drawing/DrawingWidget_Skia_GL.cc
        drawing/DrawingWindow_Skia_Vulkan.h
        drawing/DrawingWindow_Skia_Vulkan.cc
        drawing/DrawingWidget_Skia_Software.h
        drawing/DrawingWidget_Skia_Software.cc
        drawing/NonSkiaVulkanRenderer.h
        drawing/NonSkiaVulkanRenderer.cc
        SkiaFontManager.h
        SkiaFontManager.cpp)

if (IM_SYSTEM STREQUAL "Windows")
    find_package(unofficial-skia CONFIG REQUIRED)
    target_link_libraries(editcore PUBLIC unofficial::skia::skia unofficial::skia::modules::skshaper unofficial::skia::modules::skparagraph)
else ()
    set(IM_SKIA_BASE_PATH "" CACHE STRING "Skia base path")
    set(IM_SKIA_LIB_PATH "out/Shared" CACHE STRING "Skia library directory under base path")
    set(IM_SKIA_INCLUDE_PATH "include" CACHE STRING "Skia include directory under base path")
    target_include_directories(qtskia PUBLIC ${IM_SKIA_BASE_PATH}/${IM_SKIA_INCLUDE_PATH} ${IM_SKIA_BASE_PATH})
    target_link_directories(qtskia PUBLIC ${IM_SKIA_BASE_PATH}/${IM_SKIA_LIB_PATH})
    target_link_libraries(qtskia PUBLIC skia)
endif ()

target_include_directories(qtskia PUBLIC ${PROJECT_SOURCE_DIR}/sources)

# ---QtWidgets / QtGui library

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})

target_link_libraries(qtskia PRIVATE ${Qt5Gui_LIBRARIES})
target_link_libraries(qtskia PRIVATE ${Qt5Widgets_LIBRARIES})


include_directories(qtskia PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

configure_file(core-config.cpp.in ${CMAKE_BINARY_DIR}/generated/core-config.cpp)
include_directories(${CMAKE_BINARY_DIR}/generated/)

target_sources(qtskia PRIVATE
        core-config.h
        ${CMAKE_BINARY_DIR}/generated/core-config.cpp)
