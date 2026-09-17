#pragma once
#include <istream>
#include <sstream>
#include <string>
#include "ast/ast_builders/ast_builder.h"
#include "ast/ast_builders/script_builder/lexer/lexer.h"
#include "ast/ast_builders/script_builder/parser/parser.h"

namespace NTomatoInterpretato {
namespace NAst {

class ScriptAstBuilder : public IAstBuilder {
public:
    ScriptAstBuilder() = default;
    explicit ScriptAstBuilder(std::istream& input);

    ScriptAstBuilder& withSource(std::istream& input);
    void withBuiltin(const Builtins& builtins) override;
    void build(AST& ast) override;

private:
    std::string source_;
    Builtins builtins_;
};

}
}
