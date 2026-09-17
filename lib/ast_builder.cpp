#include "ast_builder.h"
#include <charconv>
#include <initializer_list>

namespace TomatoInterpretato {

namespace {

using Aliases = std::initializer_list<const char*>;

// Tags for each node kind. The first spelling is the one we believe the reference uses.
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

// Field names inside nodes
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

std::string keys_of(const Json& json) {
    if (!json.is_object()) return json.type_name();

    std::string result = "{";
    bool first = true;
    for (const auto& [key, _] : json.as_object()) {
        if (!first) result += ", ";
        result += "\"" + key + "\"";
        first = false;
    }
    return result + "}";
}

// Returns the tag key present in `json` and its payload, or nullptr
const Json* find_tag(const Json& json, Aliases tags, std::string* tag = nullptr) {
    for (const char* name : tags) {
        if (const Json* payload = json.find(name)) {
            if (tag) *tag = name;
            return payload;
        }
    }
    return nullptr;
}

// Looks a field up in the node payload first, then in the node itself (flat form,
// like {"binop": "+", "left": ..., "right": ...}). `tag` is never returned as a field.
const Json* find_field(const Json& node, const Json& payload, const std::string& tag, Aliases fields) {
    for (const char* name : fields) {
        if (const Json* field = payload.find(name)) return field;
    }
    for (const char* name : fields) {
        if (name == tag) continue;
        if (const Json* field = node.find(name)) return field;
    }
    return nullptr;
}

const Json& require_field(const Json& node, const Json& payload, const std::string& tag, Aliases fields) {
    if (const Json* field = find_field(node, payload, tag, fields)) return *field;
    throw ast_error("Node \"" + tag + "\" misses field \"" + *fields.begin() + "\": " + keys_of(node), node.position);
}

Value parse_integer(const Json& json) {
    std::string text;
    if (json.is_number()) {
        text = json.as_number().text;
    } else if (json.is_string()) {
        text = json.as_string();
    } else {
        throw ast_error("Expected integer constant, got " + json.type_name(), json.position);
    }

    Value value = 0;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || end != text.data() + text.size()) {
        throw ast_error("Bad integer constant '" + text + "'", json.position);
    }
    return value;
}

// Variable name may be given as "x" or as {"var": "x"}
std::string parse_name(const Json& json) {
    if (json.is_string()) return json.as_string();
    if (const Json* inner = find_tag(json, kVarTags)) {
        if (inner->is_string()) return inner->as_string();
    }
    throw ast_error("Expected variable name, got " + keys_of(json), json.position);
}

BinOp parse_op(const Json& json) {
    BinOp op;
    if (json.is_string() && parse_binop(json.as_string(), op)) return op;
    std::string shown = json.is_string() ? "'" + json.as_string() + "'" : json.type_name();
    throw ast_error("Unknown binary operator " + shown, json.position);
}

}  // namespace


StmtNode AstBuilder::build_program(const Json& json) const {
    if (const Json* program = json.find("program")) {
        return build_stmt(*program);
    }
    return build_stmt(json);
}

StmtNode AstBuilder::build_stmt(const Json& json) const {
    Pos pos = json.position;
    std::string tag;

    if (json.is_array()) {
        std::vector<StmtNode> body;
        for (const auto& item : json.as_array()) {
            body.push_back(build_stmt(item));
        }
        return std::make_unique<SeqStmt>(std::move(body), pos);
    }

    if (json.is_string() && json.as_string() == "skip") {
        return std::make_unique<SkipStmt>(pos);
    }

    if (!json.is_object()) {
        throw ast_error("Expected statement, got " + json.type_name(), pos);
    }

    if (const Json* p = find_tag(json, kSeqTags, &tag)) {
        if (p->is_array()) return build_stmt(*p);

        std::vector<StmtNode> body;
        body.push_back(build_stmt(require_field(json, *p, tag, kLeftFields)));
        body.push_back(build_stmt(require_field(json, *p, tag, kRightFields)));
        return std::make_unique<SeqStmt>(std::move(body), pos);
    }

    if (find_tag(json, kSkipTags)) {
        return std::make_unique<SkipStmt>(pos);
    }

    if (const Json* p = find_tag(json, kReadTags, &tag)) {
        const Json* name = p->is_object() ? find_field(json, *p, tag, kNameFields) : p;
        return std::make_unique<ReadStmt>(parse_name(name ? *name : *p), pos);
    }

    if (const Json* p = find_tag(json, kWriteTags, &tag)) {
        // {"write": E} or {"write": {"expr": E}}
        const Json* expr = p->is_object() ? p->find("expr") : nullptr;
        return std::make_unique<WriteStmt>(build_expr(expr ? *expr : *p), pos);
    }

    if (const Json* p = find_tag(json, kAssignTags, &tag)) {
        // {"assign": {"var": "x", "value": E}}  or  {"assign": "x", "value": E}
        std::string name = p->is_string() ? p->as_string() : parse_name(require_field(json, *p, tag, kNameFields));
        ExprNode value = build_expr(require_field(json, *p, tag, kValueFields));

        // IDENT BINOP "=" expr, if the reference does not desugar it itself
        if (const Json* op = find_field(json, *p, tag, kOpFields)) {
            BinOp binop = parse_op(*op);
            value = std::make_unique<BinOpExpr>(binop, std::make_unique<VariableExpr>(name, pos), std::move(value), pos);
        }
        return std::make_unique<AssignStmt>(std::move(name), std::move(value), pos);
    }

    if (const Json* p = find_tag(json, kIfTags, &tag)) {
        ExprNode cond = build_expr(require_field(json, *p, tag, kCondFields));
        StmtNode then_branch = build_stmt(require_field(json, *p, tag, kThenFields));
        StmtNode else_branch;
        if (const Json* e = find_field(json, *p, tag, kElseFields); e && !e->is_null()) {
            else_branch = build_stmt(*e);
        }
        return std::make_unique<IfStmt>(std::move(cond), std::move(then_branch), std::move(else_branch), pos);
    }

    if (const Json* p = find_tag(json, kWhileTags, &tag)) {
        ExprNode cond = build_expr(require_field(json, *p, tag, kCondFields));
        StmtNode body = build_stmt(require_field(json, *p, tag, kBodyFields));
        return std::make_unique<WhileStmt>(std::move(cond), std::move(body), pos);
    }

    if (const Json* p = find_tag(json, kDoWhileTags, &tag)) {
        StmtNode body = build_stmt(require_field(json, *p, tag, kBodyFields));
        ExprNode cond = build_expr(require_field(json, *p, tag, kCondFields));
        return std::make_unique<DoWhileStmt>(std::move(body), std::move(cond), pos);
    }

    if (const Json* p = find_tag(json, kForTags, &tag)) {
        StmtNode init = build_stmt(require_field(json, *p, tag, kInitFields));
        ExprNode cond = build_expr(require_field(json, *p, tag, kCondFields));
        StmtNode step = build_stmt(require_field(json, *p, tag, kStepFields));
        StmtNode body = build_stmt(require_field(json, *p, tag, kBodyFields));
        return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(step), std::move(body), pos);
    }

    throw ast_error("Unknown statement node " + keys_of(json), pos);
}

ExprNode AstBuilder::build_expr(const Json& json) const {
    Pos pos = json.position;
    std::string tag;

    if (json.is_number()) {
        return std::make_unique<ConstExpr>(parse_integer(json), pos);
    }

    if (!json.is_object()) {
        throw ast_error("Expected expression, got " + json.type_name(), pos);
    }

    if (const Json* p = find_tag(json, kConstTags, &tag)) {
        return std::make_unique<ConstExpr>(parse_integer(*p), pos);
    }

    if (const Json* p = find_tag(json, kVarTags, &tag)) {
        return std::make_unique<VariableExpr>(parse_name(*p), pos);
    }

    if (const Json* p = find_tag(json, kBinOpTags, &tag)) {
        // {"binop": "+", "left": E, "right": E}  or  {"binop": {"op": "+", "left": E, "right": E}}
        const Json& op = p->is_object() ? require_field(json, *p, tag, kOpFields) : *p;
        BinOp binop = parse_op(op);
        ExprNode lhs = build_expr(require_field(json, *p, tag, kLeftFields));
        ExprNode rhs = build_expr(require_field(json, *p, tag, kRightFields));
        return std::make_unique<BinOpExpr>(binop, std::move(lhs), std::move(rhs), pos);
    }

    throw ast_error("Unknown expression node " + keys_of(json), pos);
}

};
