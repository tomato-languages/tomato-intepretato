#pragma once
#include "ast/ast.h"

namespace NTomatoInterpretato::NAst {

class ScriptAstBuilder {
public:
    ScriptAstBuilder(std::istream& input = std::cin, std::ostream& output = std::cout)
        : input_(input), output_(output) {}
    AST build(std::istream& source) const;

private:
    std::istream& input_;
    std::ostream& output_;
};

}
