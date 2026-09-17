#pragma once
#include "ast.h"
#include "json.h"

namespace TomatoInterpretato {

// Converts the JSON representation of the AST into executable nodes.
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
class AstBuilder {
public:
    StmtNode build_program(const Json& json) const;
    StmtNode build_stmt(const Json& json) const;
    ExprNode build_expr(const Json& json) const;
};

};
