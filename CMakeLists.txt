cmake_minimum_required(VERSION 3.24)

project(RingPassSADX LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "RingPass-SADX currently targets Windows.")
endif()

option(RINGPASS_BUILD_SADX "Build the 32-bit SADX Mod Loader bridge" OFF)
option(RINGPASS_BUILD_ER "Build the 64-bit Elden Ring host bridge" OFF)
option(RINGPASS_BUILD_TOOLS "Build diagnostic/simulation tools" ON)

if(NOT RINGPASS_BUILD_SADX AND NOT RINGPASS_BUILD_ER)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(RINGPASS_BUILD_SADX ON)
    else()
        set(RINGPASS_BUILD_ER ON)
    endif()
endif()

if(RINGPASS_BUILD_SADX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "RingPassSADX.dll must be built as Win32/x86.")
endif()

if(RINGPASS_BUILD_ER AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "RingPassER.dll must be built as x64.")
endif()

add_subdirectory(shared)

if(RINGPASS_BUILD_SADX)
    add_subdirectory(sadx-bridge)
endif()

if(RINGPASS_BUILD_ER)
    add_subdirectory(er-bridge)
endif()

if(RINGPASS_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
