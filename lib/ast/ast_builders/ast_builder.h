#pragma once
#include "ast/ast.h"

namespace NTomatoInterpretato {
namespace NAst {

class IAstBuilder {
public:
    virtual ~IAstBuilder() = default;
    virtual void build(AST& ast) = 0;
    virtual void withBuiltin(const Builtins& builtins) = 0;
};

}
}
