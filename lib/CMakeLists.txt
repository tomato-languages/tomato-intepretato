add_library(interpreter interpreter.cpp ast_builder.cpp)

target_link_libraries(interpreter PUBLIC nlohmann_json::nlohmann_json)
