#pragma once
#include "ast/ast.h"

namespace NTomatoInterpretato {

class Interpreter {
public:
    void interpret(const NAst::AST& ast);

private:
    NAst::Environment env_;
};

}
