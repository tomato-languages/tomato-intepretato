#pragma once
#include <nlohmann/json.hpp>
#include "ast.h"

namespace TomatoInterpretato {

// Converts the JSON representation of the AST into AST nodes.
//
// Known shapes (from the reference tool):
//   {"seq": {"left": S, "right": S}}
//   {"read": "x"}
//   {"write": E}
//   {"if": {"cond": E, "then": S, "else": S}}
//   {"binop": "+", "left": E, "right": E}
//   {"var": "x"}
//   {"const": 0}
//
// Shapes of assign / while / do-while / for / skip are not confirmed yet,
// so the builder accepts several natural spellings for them (see ast_builder.cpp).
//
// seq nodes are flattened: every statement body becomes std::vector<StmtNode>.
class AstBuilder {
public:
    using Json = nlohmann::json;

    AstBuilder(const Builtins& builtins)
        : builtins_(builtins)
    {}

    void build(const Json& json, AST& ast) const;

private:
    const Builtins& builtins_;

    std::vector<StmtNode> build_body(const Json& json) const;
    void build_body(const Json& json, std::vector<StmtNode>& body) const;
    StmtNode build_stmt(const Json& json) const;
    ExprNode build_expr(const Json& json) const;

    ExprNode call_builtin(const std::string& name, ExprNode&& arg) const;
};

};
