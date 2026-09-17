#pragma once
#include <nlohmann/json.hpp>
#include "ast/ast_builders/ast_builder.h"

namespace NTomatoInterpretato {
namespace NAst {

class JsonAstBuilder : public IAstBuilder {
public:
    using Json = nlohmann::json;

    JsonAstBuilder(const Builtins& builtins)
        : builtins_(builtins)
    {}

    JsonAstBuilder& withJson(const Json& json);
    void build(AST& ast) override;

private:
    const Builtins& builtins_;
    Json json_;

    std::vector<StmtNode> build_body(const Json& json) const;
    void build_body(const Json& json, std::vector<StmtNode>& body) const;
    StmtNode build_stmt(const Json& json) const;
    ExprNode build_expr(const Json& json) const;

    ExprNode call_builtin(const std::string& name, ExprNode&& arg) const;
};

}
}
