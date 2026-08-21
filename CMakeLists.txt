cmake_minimum_required(VERSION 3.20)

project(HFTStuff
    VERSION 0.1.0
    DESCRIPTION "High-frequency trading order-book example"
    LANGUAGES CXX
)

# This project uses C++23 features such as <print> / std::println.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Do not add CMake's ZERO_CHECK regeneration project to the VS solution.
set(CMAKE_SUPPRESS_REGENERATION ON)

add_executable(HFTStuff
    Source/Main.cpp
    Source/OrderBook.cpp
    Source/OrderBook.h
    Source/Defines.h
)

target_include_directories(HFTStuff
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

if(MSVC)
    target_compile_options(HFTStuff PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
        /EHsc
    )

    # Keep the .vcxproj output directories predictable for all configurations.
    set_target_properties(HFTStuff PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()
    target_compile_options(HFTStuff PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Put CMake's helper projects into a folder in Visual Studio's Solution Explorer.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
