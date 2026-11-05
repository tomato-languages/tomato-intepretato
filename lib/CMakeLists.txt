add_library(tomato_core
    ast/ast_builders/json_builder/ast_builder.cpp
    ast/ast_builders/script_builder/ast_builder.cpp
    ast/ast_builders/script_builder/lexer/lexer.cpp
    ast/ast_builders/script_builder/parser/parser.cpp
    interpreter/interpreter.cpp
    sm/compiler/compiler.cpp
    sm/machine/machine.cpp
    sm/program/program.cpp
)

target_include_directories(tomato_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tomato_core PUBLIC nlohmann_json::nlohmann_json)
