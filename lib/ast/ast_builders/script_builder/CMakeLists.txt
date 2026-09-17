add_library(script_builder
    ast_builder.cpp
    lexer/lexer.cpp
    parser/parser.cpp
)

target_include_directories(script_builder
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
)

target_link_libraries(script_builder PUBLIC ast_builder)
