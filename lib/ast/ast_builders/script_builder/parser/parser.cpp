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

class DepthGuard {
public:
    DepthGuard(int& depth, Pos position) : depth_(depth) {
        if (depth_ >= kMaxNestingDepth) throw ast_error("Program is nested too deeply", position);
        ++depth_;
    }
    ~DepthGuard() { --depth_; }
    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;

private:
    int& depth_;
};

}

bool Parser::at(std::string_view text) const {
    return token_.kind != Token::Kind::End && token_.kind != Token::Kind::String && token_.text == text;
}

bool Parser::accept(std::string_view text) {
    if (!at(text)) return false;
    token_ = lexer_.next();
    return true;
}

void Parser::expect(std::string_view text) {
    if (!accept(text)) throw ast_error("Expected '" + std::string(text) + "', got '" + token_.text + "'", token_.position);
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
    if (!at("{")) throw ast_error("Program must start with '{'", token_.position);
    AST ast = body();
    if (token_.kind != Token::Kind::End) throw ast_error("Unexpected token after program", token_.position);
    return ast;
}

AST Parser::body() {
    DepthGuard guard(depth_, token_.position);
    AST ast;
    if (accept("{")) {
        if (at("}")) throw ast_error("Block must contain a statement", token_.position);
        do {
            if (token_.kind == Token::Kind::End) throw ast_error("Expected '}'", token_.position);
            auto part = body();
            for (auto& stmt : part) ast.push_back(std::move(stmt));
        } while (!accept("}"));
    } else {
        ast.push_back(statement());
    }
    return ast;
}

StmtNode Parser::branch() {
    DepthGuard guard(depth_, token_.position);
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
    const bool starts_with_name = token_.kind == Token::Kind::Identifier;
    auto target = postfix();
    auto* element = dynamic_cast<ElemExpr*>(target.get());
    auto* variable = starts_with_name ? dynamic_cast<VariableExpr*>(target.get()) : nullptr;
    if (element == nullptr && variable == nullptr) throw ast_error("Expected assignment target", position);
    std::string op;
    if (!accept("=")) {
        op = token_.text;
        if (token_.kind != Token::Kind::Symbol || !ValueOps::IsOperator(op)) {
            throw ast_error("Expected assignment", token_.position);
        }
        token_ = lexer_.next();
        expect("=");
    }
    auto value = expression();
    accept(";");
    if (element != nullptr) {
        return std::make_unique<SetElemStmt>(std::move(element->container), std::move(element->index),
            std::move(value), op);
    }
    std::string name = variable->var_name;
    if (!op.empty()) value = std::make_unique<BinaryOpExpr>(op, std::move(target), std::move(value), position);
    return std::make_unique<AssignStmt>(name, std::move(value));
}

ExprNode Parser::expression(int minimum) {
    DepthGuard guard(depth_, token_.position);
    auto left = postfix();
    while (token_.kind == Token::Kind::Symbol && precedence(token_.text) >= minimum) {
        auto op = token_;
        token_ = lexer_.next();
        auto right = expression(precedence(op.text) + 1);
        left = std::make_unique<BinaryOpExpr>(op.text, std::move(left), std::move(right), op.position);
    }
    return left;
}

ExprNode Parser::postfix() {
    auto value = primary();
    while (true) {
        Pos position = token_.position;
        if (accept("[")) {
            auto index = expression();
            expect("]");
            value = std::make_unique<ElemExpr>(std::move(value), std::move(index), position);
        } else if (accept(".")) {
            if (token_.kind != Token::Kind::Identifier || token_.text != "length") {
                throw ast_error("Expected 'length'", token_.position);
            }
            token_ = lexer_.next();
            value = std::make_unique<LengthExpr>(std::move(value), position);
        } else {
            return value;
        }
    }
}

ExprNode Parser::primary() {
    if (accept("(")) {
        auto value = expression();
        expect(")");
        return value;
    }
    if (token_.kind == Token::Kind::String) {
        auto node = std::make_unique<StringExpr>(token_.text, token_.position);
        token_ = lexer_.next();
        return node;
    }
    Pos start = token_.position;
    if (accept("[")) {
        if (at("]")) throw ast_error("Array must contain an element", token_.position);
        std::vector<ExprNode> elements;
        do {
            elements.push_back(expression());
        } while (accept(","));
        expect("]");
        return std::make_unique<ArrayExpr>(std::move(elements), start);
    }
    if (token_.kind == Token::Kind::Number) {
        Integer value;
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
