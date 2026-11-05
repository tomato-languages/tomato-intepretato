#pragma once
#include <algorithm>
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "ast/common/value.h"
#include "common/errors.h"
#include "common/limits.h"
#include "sm/program/program.h"

namespace NTomatoInterpretato {
namespace NAst {

using Environment = std::unordered_map<std::string, Value>;

class BaseExpr {
public:

    Pos position;
    int height = 1;

    virtual Value evaluate(Environment& env) const = 0;

    virtual void compile(NSm::Program& program) const = 0;

    BaseExpr(Pos pos)
        : position(pos)
    {}

    virtual ~BaseExpr() = default;

};
class BaseStmt {
public:

    virtual void execute(Environment& env) const = 0;

    virtual void compile(NSm::Program& program) const = 0;

    virtual ~BaseStmt() = default;

};

using StmtNode = std::unique_ptr<BaseStmt>;
using ExprNode = std::unique_ptr<BaseExpr>;

class VariableExpr : public BaseExpr {
public:

    std::string var_name;

    VariableExpr(const std::string& val, Pos position)
        : BaseExpr(position)
        , var_name(val)
    {};

    Value evaluate(Environment& env) const override {

        if (env.contains(var_name)) {
            return env[var_name];
        } else {
            throw interpret_error("Variable '" + var_name + "' is not defined", position);
        }

    };

    void compile(NSm::Program& program) const override {
        program.emit_load(var_name, position);
    }

};

class NumberExpr : public BaseExpr {

    Value value;
public:
    NumberExpr(Value val, Pos position = Pos())
        : BaseExpr(position)
        , value(val)
    {};

    Value evaluate(Environment& env) const override {
        return value;
    }

    void compile(NSm::Program& program) const override {
        program.emit_const(value, position);
    }

};

class BinaryOpExpr : public BaseExpr {
    std::string op;
    ExprNode lhs;
    ExprNode rhs;

public:
    BinaryOpExpr(const std::string& op, ExprNode&& lhs, ExprNode&& rhs, Pos position)
        : BaseExpr(position)
        , op(op)
        , lhs(std::move(lhs))
        , rhs(std::move(rhs))
    {
        height = 1 + std::max(this->lhs->height, this->rhs->height);
        if (height > kMaxNestingDepth) throw ast_error("Expression is nested too deeply", position);
    }

    Value evaluate(Environment& env) const override {
        Value left = lhs->evaluate(env);
        Value right = rhs->evaluate(env);

        return ValueOps::Apply(op, left, right);
    }

    void compile(NSm::Program& program) const override {
        lhs->compile(program);
        rhs->compile(program);
        program.emit_binop(op, position);
    }

};

class AssignStmt : public BaseStmt {
    std::string name_;
    ExprNode value_;

public:
    AssignStmt(const std::string& name, ExprNode value)
        : name_(name), value_(std::move(value)) {}

    void execute(Environment& env) const override {
        Value value = value_->evaluate(env);
        env[name_] = value;
    }

    void compile(NSm::Program& program) const override {
        value_->compile(program);
        program.emit_store(name_, value_->position);
    }
};

class ReadStmt : public BaseStmt {
    std::string name_;
    std::istream& input_;

public:
    ReadStmt(const std::string& name, std::istream& input)
        : name_(name), input_(input) {}

    void execute(Environment& env) const override {
        Value value;
        if (!(input_ >> value)) {
            throw interpret_error("read(" + name_ + "): expected an integer on input");
        }
        env[name_] = value;
    }

    void compile(NSm::Program& program) const override {
        program.emit_read();
        program.emit_store(name_);
    }
};

class WriteStmt : public BaseStmt {
    ExprNode value_;
    std::ostream& output_;

public:
    WriteStmt(ExprNode value, std::ostream& output)
        : value_(std::move(value)), output_(output) {}

    void execute(Environment& env) const override {
        Value value = value_->evaluate(env);
        output_ << value << '\n';
    }

    void compile(NSm::Program& program) const override {
        value_->compile(program);
        program.emit_write(value_->position);
    }
};

class SkipStmt : public BaseStmt {
public:
    void execute(Environment& env) const override {
    }

    void compile(NSm::Program& program) const override {}
};

class IfStmt : public BaseStmt {
public:
    IfStmt(ExprNode&& condition, std::vector<StmtNode>&& then_branch, std::vector<StmtNode>&& else_branch = std::vector<StmtNode>())
        : condition_(std::move(condition))
        , body_(std::move(then_branch))
        , else_body_(std::move(else_branch))
    {}

    void execute(Environment& env) const override {
        const auto& body = condition_->evaluate(env) != 0 ? body_ : else_body_;
        for (const auto& stmt : body) stmt->execute(env);
    }

    void compile(NSm::Program& program) const override {
        std::string else_label = program.new_label();
        std::string end_label = program.new_label();

        condition_->compile(program);
        program.emit_jump_if_zero(else_label, condition_->position);

        for (const auto& it : body_) {
            it->compile(program);
        }
        program.emit_jump(end_label);

        program.emit_label(else_label);
        for (const auto& it : else_body_) {
            it->compile(program);
        }

        program.emit_label(end_label);
    }

private:
    ExprNode condition_;

    std::vector<StmtNode> body_;
    std::vector<StmtNode> else_body_;

};

class WhileStmt : public BaseStmt {
public:
    WhileStmt(ExprNode&& condition, std::vector<StmtNode>&& body)
        : condition_(std::move(condition))
        , body_(std::move(body))
    {}

    void execute(Environment& env) const override {

        while (condition_->evaluate(env) != 0) {
            for (auto& it : body_) {
                it->execute(env);
            }
        }

    }

    void compile(NSm::Program& program) const override {
        std::string start_label = program.new_label();
        std::string end_label = program.new_label();

        program.emit_label(start_label);
        condition_->compile(program);
        program.emit_jump_if_zero(end_label, condition_->position);

        for (const auto& it : body_) {
            it->compile(program);
        }

        program.emit_jump(start_label);
        program.emit_label(end_label);

    }

private:
    ExprNode condition_;

    std::vector<StmtNode> body_;

};

class DoWhileStmt : public BaseStmt {
public:
    DoWhileStmt(std::vector<StmtNode>&& body, ExprNode&& condition)
        : condition_(std::move(condition))
        , body_(std::move(body))
    {}

    void execute(Environment& env) const override {

        do {
            for (auto& it : body_) {
                it->execute(env);
            }
        } while (condition_->evaluate(env) != 0);

    }

    void compile(NSm::Program& program) const override {
        std::string start_label = program.new_label();

        program.emit_label(start_label);
        for (const auto& it : body_) {
            it->compile(program);
        }

        condition_->compile(program);
        program.emit_jump_if_not_zero(start_label, condition_->position);
    }

private:
    ExprNode condition_;

    std::vector<StmtNode> body_;

};

class ForStmt : public BaseStmt {
public:
    ForStmt(std::vector<StmtNode>&& init, ExprNode&& condition, std::vector<StmtNode>&& step, std::vector<StmtNode>&& body)
        : init_(std::move(init))
        , condition_(std::move(condition))
        , step_(std::move(step))
        , body_(std::move(body))
    {}

    void execute(Environment& env) const override {
        for (auto& it : init_) {
            it->execute(env);
        }

        while (condition_->evaluate(env) != 0) {
            for (auto& it : body_) {
                it->execute(env);
            }
            for (auto& it : step_) {
                it->execute(env);
            }
        }

    }

    void compile(NSm::Program& program) const override {
        for (const auto& it : init_) {
            it->compile(program);
        }

        std::string start_label = program.new_label();
        std::string end_label = program.new_label();

        program.emit_label(start_label);
        condition_->compile(program);
        program.emit_jump_if_zero(end_label, condition_->position);

        for (const auto& it : body_) {
            it->compile(program);
        }

        for (const auto& it : step_) {
            it->compile(program);
        }

        program.emit_jump(start_label);
        program.emit_label(end_label);
    }

private:
    std::vector<StmtNode> init_;
    ExprNode condition_;
    std::vector<StmtNode> step_;
    std::vector<StmtNode> body_;

};

using AST = std::vector<StmtNode>;

}
}
