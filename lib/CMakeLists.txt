add_library(${PROJECT_NAME} interpreter.cpp ast.cpp parser.cpp errors.h value.cpp)
add_library(argparser argparser.cpp)