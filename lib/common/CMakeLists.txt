add_library(common utils.cpp)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(common PUBLIC json_builder script_builder)
