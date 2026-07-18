cmake_minimum_required(VERSION 3.16)
project(FastScanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt settings
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# CLI ver
add_executable(FastScannerCLI
    main.cpp
    FastScanner.cpp
    FastScanner.h
)

# GUI version (temp)
add_executable(FastScannerGUI
    gui_main.cpp
    FastScanner.cpp
    FastScanner.h
)

set_target_properties(FastScannerGUI PROPERTIES WIN32_EXECUTABLE TRUE) # Hide cmd
target_link_libraries(FastScannerGUI PRIVATE Qt6::Widgets)