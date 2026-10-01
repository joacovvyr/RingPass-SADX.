cmake_minimum_required(VERSION 3.24)

project(RingPassSADX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(shared)
add_subdirectory(sadx-bridge)
add_subdirectory(er-bridge)
