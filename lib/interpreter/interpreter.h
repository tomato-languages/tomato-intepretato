#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include "ast/ast.h"
#include "ast/ast_builders/json_builder/ast_builder.h"
#include "interpreter/functions.h"

bool interpret(std::istream& ast_input, std::istream& input, std::ostream& output);

namespace NTomatoInterpretato {

using NAst::AST;
using NAst::Environment;
using NAst::Builtins;
using NAst::JsonAstBuilder;

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

    bool interpret(std::istream& istream) {
        
        if (!istream) {
            throw interpret_error("Input stream is not valid");
        }

        nlohmann::json json;
        istream >> json;

        auto builder = JsonAstBuilder(builtins_).withJson(json);
        builder.build(ast_);

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

}
