add_library(ast_builder INTERFACE)

target_link_libraries(ast_builder INTERFACE ast)

add_subdirectory(json_builder)
add_subdirectory(script_builder)
