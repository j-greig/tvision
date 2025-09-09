cmake_minimum_required(VERSION 3.10)

# Name the project 'tvision' so source/CMakeLists.txt can use ${PROJECT_NAME}
project(test_tui_apps CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Bring in the root project to build the tvision library target
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. ${CMAKE_BINARY_DIR}/tvision-root)

# Common sources used by multiple apps
set(TV_COMMON_SOURCES
    test_pattern.cpp
    gradient.cpp
    api_ipc.cpp
)

# test_pattern app (primary playground)
add_executable(test_pattern
    test_pattern_app.cpp
    ${TV_COMMON_SOURCES}
    ascii_grid_view.cpp
    frame_file_player_view.cpp
    animated_blocks_view.cpp
    animated_gradient_view.cpp
    ansi_view.cpp
)
target_link_libraries(test_pattern PRIVATE tvision)

# simple_tui (minimal reference app)
add_executable(simple_tui
    simple_tui.cpp
)
target_link_libraries(simple_tui PRIVATE tvision)

# frame_file_player (CLI player for frame-delimited text)
add_executable(frame_file_player
    frame_file_player_main.cpp
    frame_file_player_view.cpp
)
target_link_libraries(frame_file_player PRIVATE tvision)

# ansi_viewer (optional demo; only if main exists)
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/ansi_viewer_main.cpp)
    add_executable(ansi_viewer
        ansi_viewer_main.cpp
        ansi_view.cpp
    )
    target_link_libraries(ansi_viewer PRIVATE tvision)
endif()

# paint_tui (painting MVP)
add_executable(paint_tui
    paint/paint_app.cpp
    paint/paint_canvas.cpp
    paint/paint_palette.cpp
    paint/paint_status.cpp
    paint/paint_tools.cpp
)
target_link_libraries(paint_tui PRIVATE tvision)
