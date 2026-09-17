#include "ast/ast_builders/json_builder/ast_builder.h"
#include <charconv>
#include <initializer_list>
#include <limits>
#include <unordered_map>

namespace NTomatoInterpretato {
namespace NAst {

namespace {

using Json = nlohmann::json;
using Aliases = std::initializer_list<const char*>;

const Aliases kSeqTags      = {"seq", "block", "stmts", "sequence"};
const Aliases kSkipTags     = {"skip"};
const Aliases kReadTags     = {"read"};
const Aliases kWriteTags    = {"write"};
const Aliases kAssignTags   = {"assign", "set", "=", ":="};
const Aliases kIfTags       = {"if"};
const Aliases kWhileTags    = {"while"};
const Aliases kDoWhileTags  = {"do", "dowhile", "doWhile", "do_while", "doWhileStmt"};
const Aliases kForTags      = {"for"};

const Aliases kConstTags    = {"const", "num", "number", "int"};
const Aliases kVarTags      = {"var", "ident", "id", "variable"};
const Aliases kBinOpTags    = {"binop", "op"};

const Aliases kLeftFields   = {"left", "lhs", "l", "first"};
const Aliases kRightFields  = {"right", "rhs", "r", "second"};
const Aliases kCondFields   = {"cond", "condition", "test", "expr"};
const Aliases kThenFields   = {"then", "then_branch", "thenBranch", "body"};
const Aliases kElseFields   = {"else", "else_branch", "elseBranch", "elsePart"};
const Aliases kBodyFields   = {"body", "stmt", "do"};
const Aliases kInitFields   = {"init", "initializer", "start", "pre"};
const Aliases kStepFields   = {"step", "update", "post", "next", "incr", "iter"};
const Aliases kNameFields   = {"var", "name", "ident", "id", "lhs", "left", "target", "variable"};
const Aliases kValueFields  = {"value", "expr", "rhs", "right", "val", "e"};
const Aliases kOpFields     = {"op", "binop", "operator"};

std::string describe(const Json& json) {
    std::string text = json.dump();
    if (text.size() > 80) text = text.substr(0, 77) + "...";
    return text;
}

const Json* find_key(const Json& json, const char* key) {
    if (!json.is_object()) return nullptr;
    auto it = json.find(key);
    return it == json.end() ? nullptr : &*it;
}

const Json* find_tag(const Json& json, Aliases tags, std::string* tag = nullptr) {
    for (const char* name : tags) {
        if (const Json* payload = find_key(json, name)) {
            if (tag) *tag = name;
            return payload;
        }
    }
    return nullptr;
}

const Json* find_field(const Json& node, const Json& payload, const std::string& tag, Aliases fields) {
    for (const char* name : fields) {
        if (const Json* field = find_key(payload, name)) return field;
    }
    for (const char* name : fields) {
        if (name == tag) continue;
        if (const Json* field = find_key(node, name)) return field;
    }
    return nullptr;
}

const Json& require_field(const Json& node, const Json& payload, const std::string& tag, Aliases fields) {
    if (const Json* field = find_field(node, payload, tag, fields)) return *field;
    throw ast_error("Node \"" + tag + "\" misses field \"" + *fields.begin() + "\": " + describe(node));
}

Value parse_integer(const Json& json) {
    if (json.is_number_integer() && !json.is_number_unsigned()) {
        return json.get<Value>();
    }
    if (json.is_number_unsigned() && json.get<std::uint64_t>() <= std::numeric_limits<Value>::max()) {
        return json.get<Value>();
    }
    if (json.is_string()) {
        const auto& text = json.get_ref<const std::string&>();
        Value value = 0;
        auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec == std::errc() && end == text.data() + text.size()) return value;
    }
    throw ast_error("Bad integer constant " + describe(json));
}

std::string parse_name(const Json& json) {
    if (json.is_string()) return json.get<std::string>();
    if (const Json* inner = find_tag(json, kVarTags); inner && inner->is_string()) {
        return inner->get<std::string>();
    }
    throw ast_error("Expected variable name, got " + describe(json));
}

TokenType parse_op(const Json& json) {
    static const std::unordered_map<std::string, TokenType> ops = {
        {"!!", TokenType::Or},
        {"&&", TokenType::And},
        {"==", TokenType::Equal},
        {"!=", TokenType::NotEqual},
        {"<=", TokenType::LessEqual},
        {"<", TokenType::Less},
        {">=", TokenType::GreaterEqual},
        {">", TokenType::Greater},
        {"+", TokenType::Plus},
        {"-", TokenType::Minus},
        {"*", TokenType::Star},
        {"/", TokenType::Slash},
        {"%", TokenType::Percent},
    };

    if (json.is_string()) {
        auto it = ops.find(json.get<std::string>());
        if (it != ops.end()) return it->second;
    }
    throw ast_error("Unknown binary operator " + describe(json));
}

}

JsonAstBuilder& JsonAstBuilder::withJson(const Json& json) {
    json_ = json;
    return *this;
}

void JsonAstBuilder::withBuiltin(const Builtins& builtins) {
    builtins_ = builtins;
}

void JsonAstBuilder::build(AST& ast) {
    const Json* program = find_key(json_, "program");
    for (auto& stmt : build_body(program ? *program : json_)) {
        ast.push_back(std::move(stmt));
    }
}

std::vector<StmtNode> JsonAstBuilder::build_body(const Json& json) const {
    std::vector<StmtNode> body;
    build_body(json, body);
    return body;
}

void JsonAstBuilder::build_body(const Json& json, std::vector<StmtNode>& body) const {
    std::string tag;

    if (json.is_array()) {
        for (const auto& item : json) {
            build_body(item, body);
        }
        return;
    }

    if (const Json* p = find_tag(json, kSeqTags, &tag)) {
        if (p->is_array()) {
            build_body(*p, body);
        } else {
            build_body(require_field(json, *p, tag, kLeftFields), body);
            build_body(require_field(json, *p, tag, kRightFields), body);
        }
        return;
    }

    body.push_back(build_stmt(json));
}

ExprNode JsonAstBuilder::call_builtin(const std::string& name, ExprNode&& arg) const {
    std::vector<ExprNode> args;
    args.push_back(std::move(arg));
    return ExprNode(new CallableExpr(builtins_.at(name), std::move(args)));
}

StmtNode JsonAstBuilder::build_stmt(const Json& json) const {
    Pos pos;
    std::string tag;

    if (json.is_string() && json.get<std::string>() == "skip") {
        return StmtNode(new SkipStmt());
    }

    if (!json.is_object()) {
        throw ast_error("Expected statement, got " + describe(json));
    }

    if (find_tag(json, kSkipTags)) {
        return StmtNode(new SkipStmt());
    }

    if (const Json* p = find_tag(json, kReadTags, &tag)) {
        const Json* name = p->is_object() ? find_field(json, *p, tag, kNameFields) : p;
        ExprNode var(new VariableExpr(parse_name(name ? *name : *p), pos));
        return StmtNode(new ExprStmt(call_builtin("read", std::move(var))));
    }

    if (const Json* p = find_tag(json, kWriteTags, &tag)) {
        const Json* expr = find_key(*p, "expr");
        return StmtNode(new ExprStmt(call_builtin("write", build_expr(expr ? *expr : *p))));
    }

    if (const Json* p = find_tag(json, kAssignTags, &tag)) {
        std::string name = p->is_string() ? p->get<std::string>() : parse_name(require_field(json, *p, tag, kNameFields));
        ExprNode rhs = build_expr(require_field(json, *p, tag, kValueFields));

        if (const Json* op = find_field(json, *p, tag, kOpFields)) {
            ExprNode var(new VariableExpr(name, pos));
            rhs = ExprNode(new BinaryOpExpr(parse_op(*op), std::move(var), std::move(rhs), pos));
        }

        ExprNode lhs(new VariableExpr(name, pos));
        return StmtNode(new ExprStmt(ExprNode(new AssignExpr(std::move(lhs), std::move(rhs), pos))));
    }

    if (const Json* p = find_tag(json, kIfTags, &tag)) {
        ExprNode condition = build_expr(require_field(json, *p, tag, kCondFields));
        std::vector<StmtNode> body = build_body(require_field(json, *p, tag, kThenFields));
        std::vector<StmtNode> else_body;
        if (const Json* e = find_field(json, *p, tag, kElseFields); e && !e->is_null()) {
            build_body(*e, else_body);
        }
        return StmtNode(new IfStmt(std::move(condition), std::move(body), std::move(else_body)));
    }

    if (const Json* p = find_tag(json, kWhileTags, &tag)) {
        ExprNode condition = build_expr(require_field(json, *p, tag, kCondFields));
        std::vector<StmtNode> body = build_body(require_field(json, *p, tag, kBodyFields));
        return StmtNode(new WhileStmt(std::move(condition), std::move(body)));
    }

    if (const Json* p = find_tag(json, kDoWhileTags, &tag)) {
        std::vector<StmtNode> body = build_body(require_field(json, *p, tag, kBodyFields));
        ExprNode condition = build_expr(require_field(json, *p, tag, kCondFields));
        return StmtNode(new DoWhileStmt(std::move(body), std::move(condition)));
    }

    if (const Json* p = find_tag(json, kForTags, &tag)) {
        std::vector<StmtNode> init = build_body(require_field(json, *p, tag, kInitFields));
        ExprNode condition = build_expr(require_field(json, *p, tag, kCondFields));
        std::vector<StmtNode> step = build_body(require_field(json, *p, tag, kStepFields));
        std::vector<StmtNode> body = build_body(require_field(json, *p, tag, kBodyFields));
        return StmtNode(new ForStmt(std::move(init), std::move(condition), std::move(step), std::move(body)));
    }

    throw ast_error("Unknown statement node " + describe(json));
}

ExprNode JsonAstBuilder::build_expr(const Json& json) const {
    Pos pos;
    std::string tag;

    if (json.is_number()) {
        return ExprNode(new NumberExpr(parse_integer(json), pos));
    }

    if (!json.is_object()) {
        throw ast_error("Expected expression, got " + describe(json));
    }

    if (const Json* p = find_tag(json, kConstTags, &tag)) {
        return ExprNode(new NumberExpr(parse_integer(*p), pos));
    }

    if (const Json* p = find_tag(json, kVarTags, &tag)) {
        return ExprNode(new VariableExpr(parse_name(*p), pos));
    }

    if (const Json* p = find_tag(json, kBinOpTags, &tag)) {
        const Json& op = p->is_object() ? require_field(json, *p, tag, kOpFields) : *p;
        TokenType bin_op = parse_op(op);
        ExprNode lhs = build_expr(require_field(json, *p, tag, kLeftFields));
        ExprNode rhs = build_expr(require_field(json, *p, tag, kRightFields));
        return ExprNode(new BinaryOpExpr(bin_op, std::move(lhs), std::move(rhs), pos));
    }

    throw ast_error("Unknown expression node " + describe(json));
}

}
}
