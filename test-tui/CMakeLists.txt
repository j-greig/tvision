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
add_executable(test_pattern test_pattern_app.cpp test_pattern.cpp gradient.cpp wallpaper.cpp)

# Link libraries for simple_tui
target_link_libraries(simple_tui 
    ${TVISION_DIR}/build/libtvision.a
    ${NCURSES_LIB}
)

# Link libraries for test_pattern
target_link_libraries(test_pattern
    ${TVISION_DIR}/build/libtvision.a
    ${NCURSES_LIB}
)

# Set output directory
set_target_properties(simple_tui test_pattern PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)