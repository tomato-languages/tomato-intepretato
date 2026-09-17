cmake_minimum_required(VERSION 3.14)

project(
    tomato-intepretato
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

include_directories(lib)
add_subdirectory(lib)
add_subdirectory(bin)
