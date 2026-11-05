#include "ast/ast_builders/script_builder/parser/parser.h"
#include <charconv>
#include <unordered_set>

namespace NTomatoInterpretato::NAst {

namespace {

int precedence(const std::string& op) {
    if (op == "!!") return 1;
    if (op == "&&") return 2;
    if (op == "==" || op == "!=") return 3;
    if (op == "<" || op == ">" || op == "<=" || op == ">=") return 4;
    if (op == "+" || op == "-") return 5;
    if (op == "*" || op == "/" || op == "%") return 6;
    return 0;
}

bool keyword(const std::string& text) {
    static const std::unordered_set<std::string> words{
        "read", "write", "if", "elif", "else", "while", "do", "for", "skip"
    };
    return words.contains(text);
}

}

bool Parser::accept(const std::string& text) {
    if (token_.kind == Token::Kind::End || token_.text != text) return false;
    token_ = lexer_.next();
    return true;
}

void Parser::expect(const std::string& text) {
    if (!accept(text)) throw ast_error("Expected '" + text + "', got '" + token_.text + "'", token_.position);
}

std::string Parser::identifier() {
    if (token_.kind != Token::Kind::Identifier || keyword(token_.text)) {
        throw ast_error("Expected variable name", token_.position);
    }
    std::string name = token_.text;
    token_ = lexer_.next();
    return name;
}

AST Parser::parse() {
    if (token_.text != "{") throw ast_error("Program must start with '{'", token_.position);
    AST ast = body();
    if (token_.kind != Token::Kind::End) throw ast_error("Unexpected token after program", token_.position);
    return ast;
}

AST Parser::body() {
    AST ast;
    if (accept("{")) {
        while (!accept("}")) {
            if (token_.kind == Token::Kind::End) throw ast_error("Expected '}'", token_.position);
            auto part = body();
            for (auto& stmt : part) ast.push_back(std::move(stmt));
        }
    } else {
        ast.push_back(statement());
    }
    return ast;
}

StmtNode Parser::branch() {
    expect("(");
    auto condition = expression();
    expect(")");
    auto then_body = body();
    AST else_body;
    if (accept("elif")) else_body.push_back(branch());
    else if (accept("else")) else_body = body();
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_body), std::move(else_body));
}

StmtNode Parser::statement() {
    if (accept("if")) return branch();
    if (accept("while")) {
        expect("(");
        auto condition = expression();
        expect(")");
        return std::make_unique<WhileStmt>(std::move(condition), body());
    }
    if (accept("do")) {
        auto loop_body = body();
        expect("while");
        expect("(");
        auto condition = expression();
        expect(")");
        accept(";");
        return std::make_unique<DoWhileStmt>(std::move(loop_body), std::move(condition));
    }
    if (accept("for")) {
        expect("(");
        auto init = body();
        auto condition = expression();
        expect(";");
        auto step = body();
        expect(")");
        return std::make_unique<ForStmt>(std::move(init), std::move(condition), std::move(step), body());
    }
    if (accept("skip")) {
        accept(";");
        return std::make_unique<SkipStmt>();
    }
    if (accept("read")) {
        expect("(");
        auto name = identifier();
        expect(")");
        accept(";");
        return std::make_unique<ReadStmt>(name, input_);
    }
    if (accept("write")) {
        expect("(");
        auto value = expression();
        expect(")");
        accept(";");
        return std::make_unique<WriteStmt>(std::move(value), output_);
    }
    Pos position = token_.position;
    auto name = identifier();
    std::string op;
    if (!accept("=")) {
        op = token_.text;
        if (!ValueOps::IsOperator(op)) throw ast_error("Expected assignment", token_.position);
        token_ = lexer_.next();
        expect("=");
    }
    auto value = expression();
    if (!op.empty()) {
        value = std::make_unique<BinaryOpExpr>(op,
            std::make_unique<VariableExpr>(name, position), std::move(value), position);
    }
    accept(";");
    return std::make_unique<AssignStmt>(name, std::move(value));
}

ExprNode Parser::expression(int minimum) {
    auto left = primary();
    while (precedence(token_.text) >= minimum) {
        auto op = token_;
        token_ = lexer_.next();
        auto right = expression(precedence(op.text) + 1);
        left = std::make_unique<BinaryOpExpr>(op.text, std::move(left), std::move(right), op.position);
    }
    return left;
}

ExprNode Parser::primary() {
    if (accept("(")) {
        auto value = expression();
        expect(")");
        return value;
    }
    if (token_.kind == Token::Kind::Number) {
        Value value;
        auto result = std::from_chars(token_.text.data(), token_.text.data() + token_.text.size(), value);
        if (result.ec != std::errc()) throw ast_error("Integer constant is out of range", token_.position);
        auto node = std::make_unique<NumberExpr>(value, token_.position);
        token_ = lexer_.next();
        return node;
    }
    Pos position = token_.position;
    return std::make_unique<VariableExpr>(identifier(), position);
}

}
