#pragma once
#include <iostream>
#include "ast.h"
#include "ast_builder.h"
#include "json.h"

namespace TomatoInterpretato {

class Interpreter {
public:

    Interpreter(std::istream& input = std::cin, std::ostream& output = std::cout)
        : env_(input, output)
    {}

    // Reads one JSON AST from `ast_stream` and executes it.
    // `ast_stream` may be the same stream as the program input:
    // everything after the JSON value is left for `read`.
    void interpret(std::istream& ast_stream);

private:

    Environment env_;
    AstBuilder builder_;

};

};
