add_library(interpreter interpreter.cpp)

target_include_directories(interpreter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(interpreter PUBLIC json_builder nlohmann_json::nlohmann_json)
