#pragma once
#include <nlohmann/json.hpp>
#include "ast.h"

namespace TomatoInterpretato {

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
