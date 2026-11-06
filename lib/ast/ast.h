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

protected:
    void set_children_height(int children_height) {
        height = children_height + 1;
        if (height > kMaxNestingDepth) throw ast_error("Expression is nested too deeply", position);
    }

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

    Integer value;
public:
    NumberExpr(Integer val, Pos position = Pos())
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
        set_children_height(std::max(this->lhs->height, this->rhs->height));
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

class StringExpr : public BaseExpr {
    std::string text;

public:
    StringExpr(const std::string& text, Pos position = Pos())
        : BaseExpr(position)
        , text(text)
    {}

    Value evaluate(Environment& env) const override {
        return std::make_shared<std::string>(text);
    }

    void compile(NSm::Program& program) const override {
        program.emit_string(text, position);
    }

};

class ArrayExpr : public BaseExpr {
    std::vector<ExprNode> elements;

public:
    ArrayExpr(std::vector<ExprNode>&& elements, Pos position = Pos())
        : BaseExpr(position)
        , elements(std::move(elements))
    {
        int children_height = 0;
        for (const auto& element : this->elements) children_height = std::max(children_height, element->height);
        set_children_height(children_height);
    }

    Value evaluate(Environment& env) const override {
        auto array = std::make_shared<ArrayData>();
        array->elements.reserve(elements.size());
        for (const auto& element : elements) array->elements.push_back(element->evaluate(env));
        return array;
    }

    void compile(NSm::Program& program) const override {
        for (const auto& element : elements) element->compile(program);
        program.emit_array(static_cast<Integer>(elements.size()), position);
    }

};

class ElemExpr : public BaseExpr {
public:
    ExprNode container;
    ExprNode index;

    ElemExpr(ExprNode&& container, ExprNode&& index, Pos position = Pos())
        : BaseExpr(position)
        , container(std::move(container))
        , index(std::move(index))
    {
        set_children_height(std::max(this->container->height, this->index->height));
    }

    Value evaluate(Environment& env) const override {
        Value target = container->evaluate(env);
        Value position_value = index->evaluate(env);
        return ValueOps::Elem(target, position_value);
    }

    void compile(NSm::Program& program) const override {
        container->compile(program);
        index->compile(program);
        program.emit_elem(position);
    }

};

class LengthExpr : public BaseExpr {
    ExprNode value;

public:
    LengthExpr(ExprNode&& value, Pos position = Pos())
        : BaseExpr(position)
        , value(std::move(value))
    {
        set_children_height(this->value->height);
    }

    Value evaluate(Environment& env) const override {
        return ValueOps::Length(value->evaluate(env));
    }

    void compile(NSm::Program& program) const override {
        value->compile(program);
        program.emit_length(position);
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

class SetElemStmt : public BaseStmt {
    ExprNode container_;
    ExprNode index_;
    ExprNode value_;
    std::string op_;

public:
    SetElemStmt(ExprNode&& container, ExprNode&& index, ExprNode&& value, const std::string& op = "")
        : container_(std::move(container))
        , index_(std::move(index))
        , value_(std::move(value))
        , op_(op)
    {}

    void execute(Environment& env) const override {
        Value target = container_->evaluate(env);
        Value position = index_->evaluate(env);
        if (op_.empty()) {
            ValueOps::SetElem(target, position, value_->evaluate(env));
            return;
        }
        Value current = ValueOps::Elem(target, position);
        Value operand = value_->evaluate(env);
        ValueOps::SetElem(target, position, ValueOps::Apply(op_, current, operand));
    }

    void compile(NSm::Program& program) const override {
        container_->compile(program);
        index_->compile(program);
        if (!op_.empty()) {
            container_->compile(program);
            index_->compile(program);
            program.emit_elem(value_->position);
        }
        value_->compile(program);
        if (!op_.empty()) program.emit_binop(op_, value_->position);
        program.emit_set(value_->position);
    }
};

class ReadStmt : public BaseStmt {
    std::string name_;
    std::istream& input_;

public:
    ReadStmt(const std::string& name, std::istream& input)
        : name_(name), input_(input) {}

    void execute(Environment& env) const override {
        Integer value;
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
        ValueOps::Write(output_, value);
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
        const auto& body = ValueOps::IsTrue(condition_->evaluate(env)) ? body_ : else_body_;
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

        while (ValueOps::IsTrue(condition_->evaluate(env))) {
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
        } while (ValueOps::IsTrue(condition_->evaluate(env)));

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

        while (ValueOps::IsTrue(condition_->evaluate(env))) {
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
