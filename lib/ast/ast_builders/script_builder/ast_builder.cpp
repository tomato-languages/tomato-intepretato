#include "ast/ast_builders/script_builder/ast_builder.h"
#include "common/errors.h"

#include <iterator>

namespace NTomatoInterpretato {
namespace NAst {

ScriptAstBuilder::ScriptAstBuilder(std::istream& input) {
    withSource(input);
}

ScriptAstBuilder& ScriptAstBuilder::withSource(std::istream& input) {
    if (!input) {
        throw interpret_error("Input stream is not valid");
    }
    source_.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return *this;
}

void ScriptAstBuilder::withBuiltin(const Builtins& builtins) {
    builtins_ = builtins;
}

void ScriptAstBuilder::build(AST& ast) {
    std::istringstream stream(source_);
    Lexer lexer(stream);
    Parser parser(lexer, ast, builtins_);
    parser.parse();
}

}
}
