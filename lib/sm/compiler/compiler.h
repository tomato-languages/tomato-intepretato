#pragma once
#include "ast/ast.h"
#include "sm/program/program.h"

namespace NTomatoInterpretato::NSm {

class Compiler {
public:
    Program compile(const NAst::AST& ast) const;
};

}
