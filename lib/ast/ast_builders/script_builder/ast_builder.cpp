#include "ast/ast_builders/script_builder/ast_builder.h"
#include "ast/ast_builders/script_builder/lexer/lexer.h"
#include "ast/ast_builders/script_builder/parser/parser.h"

namespace NTomatoInterpretato::NAst {

AST ScriptAstBuilder::build(std::istream& source) const {
    Lexer lexer(source);
    return Parser(lexer, input_, output_).parse();
}

}
