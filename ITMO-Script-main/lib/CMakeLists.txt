add_library(itmoscript interpreter.cpp lexer.cpp ast.cpp parser.cpp errors.h value.cpp)
add_library(argparser argparser.cpp)