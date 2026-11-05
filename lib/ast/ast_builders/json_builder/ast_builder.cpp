#include "ast/ast_builders/json_builder/ast_builder.h"
#include <limits>

namespace NTomatoInterpretato::NAst {

using Json = nlohmann::json;

namespace {

void check_depth(int depth) {
    if (depth > kMaxNestingDepth) throw ast_error("Program is nested too deeply");
}

std::string describe(const Json& json) {
    if (json.is_array()) return "array";
    if (!json.is_object()) return json.dump();
    std::string keys;
    for (auto it = json.begin(); it != json.end(); ++it) {
        if (!keys.empty()) keys += ", ";
        keys += it.key();
    }
    return "object with keys: " + keys;
}

}

AST JsonAstBuilder::build(const Json& json) const {
    AST body;
    build_body(json.contains("program") ? json.at("program") : json, body, 0);
    return body;
}

void JsonAstBuilder::build_body(const Json& json, AST& body, int depth) const {
    check_depth(depth);
    const Json* current = &json;
    while (current->contains("seq") && !current->at("seq").is_array()) {
        const auto& seq = current->at("seq");
        build_body(seq.at("left"), body, depth + 1);
        current = &seq.at("right");
    }
    if (current->is_array()) {
        for (const auto& stmt : *current) build_body(stmt, body, depth + 1);
    } else if (current->contains("seq")) {
        build_body(current->at("seq"), body, depth + 1);
    } else {
        body.push_back(build_stmt(*current, depth));
    }
}

StmtNode JsonAstBuilder::build_stmt(const Json& json, int depth) const {
    if (json == "skip" || json.contains("skip")) return std::make_unique<SkipStmt>();
    if (json.contains("read")) {
        return std::make_unique<ReadStmt>(json.at("read").get<std::string>(), input_);
    }
    if (json.contains("write")) {
        return std::make_unique<WriteStmt>(build_expr(json.at("write"), depth + 1), output_);
    }
    if (json.contains("assn")) {
        const auto& assign = json.at("assn");
        return std::make_unique<AssignStmt>(assign.at("dst").get<std::string>(),
            build_expr(assign.at("src"), depth + 1));
    }
    if (json.contains("if")) {
        const auto& branch = json.at("if");
        AST then_body, else_body;
        build_body(branch.at("then"), then_body, depth + 1);
        if (branch.contains("else") && !branch.at("else").is_null()) {
            build_body(branch.at("else"), else_body, depth + 1);
        }
        return std::make_unique<IfStmt>(build_expr(branch.at("cond"), depth + 1),
            std::move(then_body), std::move(else_body));
    }
    if (json.contains("while")) {
        const auto& loop = json.at("while");
        AST body;
        build_body(loop.at("body"), body, depth + 1);
        return std::make_unique<WhileStmt>(build_expr(loop.at("cond"), depth + 1), std::move(body));
    }
    if (json.contains("do")) {
        const auto& loop = json.at("do");
        AST body;
        build_body(loop.at("body"), body, depth + 1);
        return std::make_unique<DoWhileStmt>(std::move(body), build_expr(loop.at("cond"), depth + 1));
    }
    if (json.contains("for")) {
        const auto& loop = json.at("for");
        AST init, step, body;
        build_body(loop.at("init"), init, depth + 1);
        build_body(loop.at("step"), step, depth + 1);
        build_body(loop.at("body"), body, depth + 1);
        return std::make_unique<ForStmt>(std::move(init), build_expr(loop.at("cond"), depth + 1),
            std::move(step), std::move(body));
    }
    throw ast_error("Unknown statement: " + describe(json));
}

ExprNode JsonAstBuilder::build_expr(const Json& json, int depth) const {
    check_depth(depth);
    if (json.contains("const")) {
        const auto& value = json.at("const");
        if (!value.is_number_integer() || (value.is_number_unsigned() &&
            value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<Value>::max()))) {
            throw ast_error("Expected a 64-bit integer: " + describe(value));
        }
        return std::make_unique<NumberExpr>(value.get<Value>());
    }
    if (json.contains("var")) {
        return std::make_unique<VariableExpr>(json.at("var").get<std::string>(), Pos());
    }
    if (json.contains("binop")) {
        std::string op = json.at("binop").get<std::string>();
        if (!ValueOps::IsOperator(op)) throw ast_error("Unknown binary operator: " + op);
        return std::make_unique<BinaryOpExpr>(op, build_expr(json.at("left"), depth + 1),
            build_expr(json.at("right"), depth + 1), Pos());
    }
    throw ast_error("Unknown expression: " + describe(json));
}

}
