#include "ast/ast_builders/json_builder/ast_builder.h"
#include <limits>

namespace NTomatoInterpretato::NAst {

using Json = nlohmann::json;

AST JsonAstBuilder::build(const Json& json) const {
    AST body;
    build_body(json.contains("program") ? json.at("program") : json, body);
    return body;
}

void JsonAstBuilder::build_body(const Json& json, AST& body) const {
    if (json.is_array()) {
        for (const auto& stmt : json) build_body(stmt, body);
    } else if (json.contains("seq")) {
        const auto& seq = json.at("seq");
        if (seq.is_array()) {
            build_body(seq, body);
        } else {
            build_body(seq.at("left"), body);
            build_body(seq.at("right"), body);
        }
    } else {
        body.push_back(build_stmt(json));
    }
}

StmtNode JsonAstBuilder::build_stmt(const Json& json) const {
    if (json == "skip" || json.contains("skip")) return std::make_unique<SkipStmt>();
    if (json.contains("read")) {
        return std::make_unique<ReadStmt>(json.at("read").get<std::string>(), input_);
    }
    if (json.contains("write")) {
        return std::make_unique<WriteStmt>(build_expr(json.at("write")), output_);
    }
    if (json.contains("assign")) {
        const auto& assign = json.at("assign");
        std::string name = assign.at("var").get<std::string>();
        auto value = build_expr(assign.at("value"));
        if (assign.contains("op")) {
            std::string op = assign.at("op").get<std::string>();
            if (!ValueOps::IsOperator(op)) throw ast_error("Unknown binary operator: " + op);
            value = std::make_unique<BinaryOpExpr>(op,
                std::make_unique<VariableExpr>(name, Pos()), std::move(value), Pos());
        }
        return std::make_unique<AssignStmt>(name, std::move(value));
    }
    if (json.contains("if")) {
        const auto& branch = json.at("if");
        AST then_body, else_body;
        build_body(branch.at("then"), then_body);
        if (branch.contains("else") && !branch.at("else").is_null()) {
            build_body(branch.at("else"), else_body);
        }
        return std::make_unique<IfStmt>(build_expr(branch.at("cond")),
            std::move(then_body), std::move(else_body));
    }
    if (json.contains("while")) {
        const auto& loop = json.at("while");
        AST body;
        build_body(loop.at("body"), body);
        return std::make_unique<WhileStmt>(build_expr(loop.at("cond")), std::move(body));
    }
    if (json.contains("do")) {
        const auto& loop = json.at("do");
        AST body;
        build_body(loop.at("body"), body);
        return std::make_unique<DoWhileStmt>(std::move(body), build_expr(loop.at("cond")));
    }
    if (json.contains("for")) {
        const auto& loop = json.at("for");
        AST init, step, body;
        build_body(loop.at("init"), init);
        build_body(loop.at("step"), step);
        build_body(loop.at("body"), body);
        return std::make_unique<ForStmt>(std::move(init), build_expr(loop.at("cond")),
            std::move(step), std::move(body));
    }
    throw ast_error("Unknown statement: " + json.dump());
}

ExprNode JsonAstBuilder::build_expr(const Json& json) const {
    if (json.contains("const")) {
        const auto& value = json.at("const");
        if (!value.is_number_integer() || (value.is_number_unsigned() &&
            value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<Value>::max()))) {
            throw ast_error("Expected a 64-bit integer: " + value.dump());
        }
        return std::make_unique<NumberExpr>(value.get<Value>());
    }
    if (json.contains("var")) {
        return std::make_unique<VariableExpr>(json.at("var").get<std::string>(), Pos());
    }
    if (json.contains("binop")) {
        std::string op = json.at("binop").get<std::string>();
        if (!ValueOps::IsOperator(op)) throw ast_error("Unknown binary operator: " + op);
        return std::make_unique<BinaryOpExpr>(op, build_expr(json.at("left")),
            build_expr(json.at("right")), Pos());
    }
    throw ast_error("Unknown expression: " + json.dump());
}

}
