add_library(json_builder ast_builder.cpp)

target_link_libraries(json_builder PUBLIC ast_builder nlohmann_json::nlohmann_json)
