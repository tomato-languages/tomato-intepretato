#pragma once
#include <nlohmann/json.hpp>
#include "ast/ast.h"

namespace NTomatoInterpretato::NAst {

class JsonAstBuilder {
public:
    JsonAstBuilder(std::istream& input = std::cin, std::ostream& output = std::cout)
        : input_(input), output_(output) {}
    AST build(const nlohmann::json& json) const;

private:
    std::istream& input_;
    std::ostream& output_;
    void build_body(const nlohmann::json& json, AST& body, int depth) const;
    StmtNode build_stmt(const nlohmann::json& json, int depth) const;
    ExprNode build_expr(const nlohmann::json& json, int depth) const;
};

}
