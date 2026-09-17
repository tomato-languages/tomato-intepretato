#pragma once
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "errors.h"

namespace TomatoInterpretato {

using Value = std::int64_t;

struct Environment {
    std::unordered_map<std::string, Value> variables;
    std::istream& input;
    std::ostream& output;

    Environment(std::istream& in, std::ostream& out)
        : input(in)
        , output(out)
    {}
};

enum class BinOp {
    Or,            // !!
    And,           // &&
    Equal,         // ==
    NotEqual,      // !=
    LessEqual,     // <=
    Less,          // <
    GreaterEqual,  // >=
    Greater,       // >
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %
};

// Returns false if `text` is not a BINOP of the language
bool parse_binop(const std::string& text, BinOp& op);
std::string binop_to_string(BinOp op);
Value apply_binop(BinOp op, Value lhs, Value rhs, Pos position);


// Expressions

class BaseExpr {
public:

    Pos position;

    explicit BaseExpr(Pos pos)
        : position(pos)
    {}

    virtual Value evaluate(Environment& env) const = 0;

    virtual ~BaseExpr() = default;

};

using ExprNode = std::unique_ptr<BaseExpr>;

class ConstExpr : public BaseExpr {
    Value value_;

public:
    ConstExpr(Value value, Pos position)
        : BaseExpr(position)
        , value_(value)
    {}

    Value evaluate(Environment&) const override {
        return value_;
    }
};

class VariableExpr : public BaseExpr {
    std::string name_;

public:
    VariableExpr(std::string name, Pos position)
        : BaseExpr(position)
        , name_(std::move(name))
    {}

    Value evaluate(Environment& env) const override {
        auto it = env.variables.find(name_);
        if (it == env.variables.end()) {
            throw interpret_error("Variable '" + name_ + "' is not defined", position);
        }
        return it->second;
    }
};

class BinOpExpr : public BaseExpr {
    BinOp op_;
    ExprNode lhs_;
    ExprNode rhs_;

public:
    BinOpExpr(BinOp op, ExprNode&& lhs, ExprNode&& rhs, Pos position)
        : BaseExpr(position)
        , op_(op)
        , lhs_(std::move(lhs))
        , rhs_(std::move(rhs))
    {}

    Value evaluate(Environment& env) const override {
        // both operands are always evaluated: && and !! are not short-circuit
        Value lhs = lhs_->evaluate(env);
        Value rhs = rhs_->evaluate(env);
        return apply_binop(op_, lhs, rhs, position);
    }
};


// Statements

class BaseStmt {
public:

    Pos position;

    explicit BaseStmt(Pos pos)
        : position(pos)
    {}

    virtual void execute(Environment& env) const = 0;

    virtual ~BaseStmt() = default;

};

using StmtNode = std::unique_ptr<BaseStmt>;

class SkipStmt : public BaseStmt {
public:
    explicit SkipStmt(Pos position)
        : BaseStmt(position)
    {}

    void execute(Environment&) const override {}
};

class ReadStmt : public BaseStmt {
    std::string name_;

public:
    ReadStmt(std::string name, Pos position)
        : BaseStmt(position)
        , name_(std::move(name))
    {}

    void execute(Environment& env) const override {
        Value value;
        if (!(env.input >> value)) {
            throw interpret_error("read(" + name_ + "): expected an integer on input", position);
        }
        env.variables[name_] = value;
    }
};

class WriteStmt : public BaseStmt {
    ExprNode expr_;

public:
    WriteStmt(ExprNode&& expr, Pos position)
        : BaseStmt(position)
        , expr_(std::move(expr))
    {}

    void execute(Environment& env) const override {
        env.output << expr_->evaluate(env) << '\n';
    }
};

class AssignStmt : public BaseStmt {
    std::string name_;
    ExprNode expr_;

public:
    AssignStmt(std::string name, ExprNode&& expr, Pos position)
        : BaseStmt(position)
        , name_(std::move(name))
        , expr_(std::move(expr))
    {}

    void execute(Environment& env) const override {
        env.variables[name_] = expr_->evaluate(env);
    }
};

class SeqStmt : public BaseStmt {
    std::vector<StmtNode> body_;

public:
    SeqStmt(std::vector<StmtNode>&& body, Pos position)
        : BaseStmt(position)
        , body_(std::move(body))
    {}

    void execute(Environment& env) const override {
        for (const auto& stmt : body_) {
            stmt->execute(env);
        }
    }
};

class IfStmt : public BaseStmt {
    ExprNode cond_;
    StmtNode then_;
    StmtNode else_;  // may be null

public:
    IfStmt(ExprNode&& cond, StmtNode&& then_branch, StmtNode&& else_branch, Pos position)
        : BaseStmt(position)
        , cond_(std::move(cond))
        , then_(std::move(then_branch))
        , else_(std::move(else_branch))
    {}

    void execute(Environment& env) const override {
        if (cond_->evaluate(env) != 0) {
            then_->execute(env);
        } else if (else_) {
            else_->execute(env);
        }
    }
};

class WhileStmt : public BaseStmt {
    ExprNode cond_;
    StmtNode body_;

public:
    WhileStmt(ExprNode&& cond, StmtNode&& body, Pos position)
        : BaseStmt(position)
        , cond_(std::move(cond))
        , body_(std::move(body))
    {}

    void execute(Environment& env) const override {
        while (cond_->evaluate(env) != 0) {
            body_->execute(env);
        }
    }
};

class DoWhileStmt : public BaseStmt {
    StmtNode body_;
    ExprNode cond_;

public:
    DoWhileStmt(StmtNode&& body, ExprNode&& cond, Pos position)
        : BaseStmt(position)
        , body_(std::move(body))
        , cond_(std::move(cond))
    {}

    void execute(Environment& env) const override {
        do {
            body_->execute(env);
        } while (cond_->evaluate(env) != 0);
    }
};

class ForStmt : public BaseStmt {
    StmtNode init_;
    ExprNode cond_;
    StmtNode step_;
    StmtNode body_;

public:
    ForStmt(StmtNode&& init, ExprNode&& cond, StmtNode&& step, StmtNode&& body, Pos position)
        : BaseStmt(position)
        , init_(std::move(init))
        , cond_(std::move(cond))
        , step_(std::move(step))
        , body_(std::move(body))
    {}

    void execute(Environment& env) const override {
        for (init_->execute(env); cond_->evaluate(env) != 0; step_->execute(env)) {
            body_->execute(env);
        }
    }
};

};
