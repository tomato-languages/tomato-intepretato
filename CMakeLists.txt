cmake_minimum_required(VERSION 3.12)

project(
    tomato-intepretato
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

include_directories(lib)
add_subdirectory(lib)
add_subdirectory(bin)
