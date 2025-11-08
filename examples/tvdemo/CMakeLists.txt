tv_app_add_generic(tvdemo)

# Try to find OpenCV for webcam support (optional)
find_package(OpenCV QUIET)

if(OpenCV_FOUND)
    message(STATUS "OpenCV found - enabling webcam support with real camera")
    target_compile_definitions(tvdemo PRIVATE HAVE_OPENCV)
    target_link_libraries(tvdemo PRIVATE ${OpenCV_LIBS})
    target_include_directories(tvdemo PRIVATE ${OpenCV_INCLUDE_DIRS})
else()
    message(STATUS "OpenCV not found - webcam will use test pattern")
    message(STATUS "To enable webcam: install OpenCV and rebuild")
    message(STATUS "  macOS: brew install opencv")
    message(STATUS "  Linux: sudo apt-get install libopencv-dev")
endif()

if (WIN32)
    target_link_libraries(tvdemo PRIVATE
        psapi # Required by 'GetProcessMemoryInfo' (gadgets.cpp)
    )
endif()
