#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include "ast.h"
#include "ast_builder.h"
#include "functions.h"
bool interpret(std::istream& ast_input, std::istream& input, std::ostream& output);

namespace TomatoInterpretato {
    
class Interpreter {
public:

    Interpreter(std::istream& input = std::cin, std::ostream& output = std::cout) 
        : output_(output)
        , ast_()
        , env_()
        , builtins_()
    {

        builtins_["read"] = std::make_shared<ReadFunction>(input);
        builtins_["write"] = std::make_shared<WriteFunction>(output_);

    }

    // Reads one JSON AST from `istream` and executes it.
    // `istream` may be the program input itself: everything after the JSON value is left for `read`.
    bool interpret(std::istream& istream) {
        
        if (!istream) {
            throw interpret_error("Input stream is not valid");
        }

        nlohmann::json json;
        istream >> json;

        AstBuilder builder(builtins_);
        builder.build(json, ast_);

        while (!ast_.empty()) {
            ast_.front()->execute(env_);
            ast_.pop_front();
        }

        output_.flush();

        return true;

    }


private:

    std::ostream& output_;

    AST ast_;
    Environment env_;
    Builtins builtins_;

};

};
