#pragma once
#include "ast/ast.h"
#include "ast/ast_builders/script_builder/lexer/lexer.h"

namespace NTomatoInterpretato::NAst {

class Parser {
public:
    Parser(Lexer& lexer, std::istream& input, std::ostream& output)
        : lexer_(lexer), input_(input), output_(output), token_(lexer_.next()) {}
    AST parse();

private:
    Lexer& lexer_;
    std::istream& input_;
    std::ostream& output_;
    Token token_;

    bool accept(const std::string& text);
    void expect(const std::string& text);
    std::string identifier();
    AST body();
    StmtNode statement();
    StmtNode branch();
    ExprNode expression(int minimum = 1);
    ExprNode primary();
};

}
