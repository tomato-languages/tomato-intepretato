#pragma once
#include <iostream>
#include <memory>
#include "ast/ast.h"
#include "ast/ast_builders/ast_builder.h"
#include "interpreter/functions.h"
#include "common/errors.h"

bool interpret(std::istream& ast_input, std::istream& input, std::ostream& output);

namespace NTomatoInterpretato {

using NAst::AST;
using NAst::Environment;
using NAst::Builtins;

class Interpreter {
public:

    Interpreter(std::istream& input = std::cin, std::ostream& output = std::cout) 
        : output_(output)
        , env_()
        , builtins_()
    {

        builtins_["read"] = std::make_shared<ReadFunction>(input);
        builtins_["write"] = std::make_shared<WriteFunction>(output_);

    }

    bool interpret(const std::shared_ptr<NAst::IAstBuilder>& builder) {
        if (!builder) [[unlikely]] {
            return false;
        }

        builder->withBuiltin(builtins_);

        AST ast;
        builder->build(ast);

        while (!ast.empty()) {
            ast.front()->execute(env_);
            ast.pop_front();
        }

        output_.flush();

        return true;

    }


private:

    std::ostream& output_;

    Environment env_;
    Builtins builtins_;

};

}
