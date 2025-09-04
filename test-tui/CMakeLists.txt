cmake_minimum_required(VERSION 3.5)
project(SimpleTUI)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the parent directory to find the tvision library
set(TVISION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Include directories
include_directories(${TVISION_DIR}/include)
include_directories(${TVISION_DIR}/include/tvision)
include_directories(${TVISION_DIR}/include/tvision/compat/borland)

# Find ncurses
if(APPLE)
    find_library(NCURSES_LIB ncurses REQUIRED)
else()
    find_library(NCURSES_LIB ncursesw REQUIRED)
endif()

# Add executables
add_executable(simple_tui simple_tui.cpp)
add_executable(test_pattern 
    test_pattern_app.cpp 
    test_pattern.cpp 
    gradient.cpp 
    wallpaper.cpp
    ansi_view.cpp
    frame_file_player_view.cpp
)

# Link libraries for simple_tui
target_link_libraries(simple_tui 
    ${TVISION_DIR}/build/libtvision.a
    ${NCURSES_LIB}
)

# Find threading library for animations
find_package(Threads REQUIRED)

# Link libraries for test_pattern
target_link_libraries(test_pattern
    ${TVISION_DIR}/build/libtvision.a
    ${NCURSES_LIB}
    Threads::Threads
)

# Set output directory
set_target_properties(simple_tui test_pattern PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Frame-file animation player (MVP)
add_executable(frame_file_player
    frame_file_player_main.cpp
    frame_file_player_view.cpp
)

target_link_libraries(frame_file_player
    ${TVISION_DIR}/build/libtvision.a
    ${NCURSES_LIB}
)

set_target_properties(frame_file_player PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ANSI viewer microapp (MVP)
add_executable(ansi_viewer
    ansi_viewer_main.cpp
    ansi_view.cpp
)

target_link_libraries(ansi_viewer
    ${TVISION_DIR}/build/libtvision.a
    ${NCURSES_LIB}
)

set_target_properties(ansi_viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Paint MVP app
add_executable(paint_tui
    paint/paint_app.cpp
    paint/paint_canvas.cpp
)

target_link_libraries(paint_tui
    ${TVISION_DIR}/build/libtvision.a
    ${NCURSES_LIB}
)

set_target_properties(paint_tui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
