cmake_minimum_required(VERSION 3.24)

project(RingPassSADX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "RingPass-SADX currently targets Windows.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR
        "SADX mods must be built as 32-bit x86. "
        "With Visual Studio use: cmake -A Win32 ...")
endif()

add_subdirectory(shared)
add_subdirectory(sadx-bridge)
add_subdirectory(er-bridge)
add_subdirectory(tools)
