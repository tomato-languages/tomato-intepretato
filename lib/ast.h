#pragma once
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include "value.h"
#include "errors.h"

namespace TomatoInterpretato {


enum class TokenType {
    // Ключевые слова
    Read, Write,
    If, Elif, Else,
    While, Do, For,
    Skip,

    // Общие
    Identifier,

    // Логические
    And,         // &&
    Or,          // !!

    // Арифметические
    Plus,        // +
    Minus,       // -
    Star,        // *
    Slash,       // /
    Percent,     // %

    // Присваивания
    Assign,          // =
    OrAssign,        // !!=
    AndAssign,       // &&=
    PlusAssign,      // +=
    MinusAssign,     // -=
    StarAssign,      // *=
    SlashAssign,     // /=
    PercentAssign,   // %=

    // Сравнения
    Equal,           // ==
    NotEqual,        // !=
    Less,            // <
    Greater,         // >
    LessEqual,       // <=
    GreaterEqual,    // >=

    Semicolon,    // ;
    LParen,       // (
    RParen,       // )
    LBrace,       // {
    RBrace,       // }

    // Литералы
    Number,

    // Специальные
    Comment,
    EndOfFile,
    Unknown

};

using Environment = std::unordered_map<std::string, Value>;


class BaseExpr {
public:

    Pos position;

    virtual Value evaluate(Environment& env) const = 0;

    BaseExpr(Pos pos)
        : position(pos)
    {}

    virtual ~BaseExpr() = default;

};
class BaseStmt {
public:

    virtual bool execute(Environment& env) = 0;

    virtual ~BaseStmt() = default;

};


using StmtNode = std::unique_ptr<BaseStmt>;
using ExprNode = std::unique_ptr<BaseExpr>;


// Expressions


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




};

class AssignExpr : public BaseExpr {
    ExprNode lhs;
    ExprNode rhs;

public:
    AssignExpr(ExprNode&& lhs, ExprNode&& rhs, Pos position)
        : BaseExpr(position)
        , lhs(std::move(lhs)), rhs(std::move(rhs))
    {}

    Value evaluate(Environment& env) const override {

        if (auto var_node = dynamic_cast<VariableExpr*>(lhs.get())) {
            Value to_assign = rhs->evaluate(env);

            return env[var_node->var_name] = to_assign;

        } else {
            throw interpret_error(lhs->position.to_string() + "Left-hand side of assignment must be a variable.");
        }

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

};


// Built-in functions (read, write) are implemented in functions.h
class Function {
public:

    virtual Value call(const std::vector<ExprNode>& args_val, Environment& env) const = 0;

    virtual ~Function() = default;

};

using Builtins = std::unordered_map<std::string, std::shared_ptr<Function>>;


class BinaryOpExpr : public BaseExpr {
    TokenType op;
    ExprNode lhs;
    ExprNode rhs;

public:
    BinaryOpExpr(TokenType op, ExprNode&& lhs, ExprNode&& rhs, Pos position)
        : BaseExpr(position)
        , op(op)
        , lhs(std::move(lhs))
        , rhs(std::move(rhs))
    {}

    Value evaluate(Environment& env) const override {
        Value left = lhs->evaluate(env);
        Value right = rhs->evaluate(env);

        if (op == TokenType::Plus) {
            return ValueOps::Additive{}(left, right);
        } else if (op == TokenType::Minus) {
            return ValueOps::Substract{}(left, right);
        } else if (op == TokenType::Star) {
            return ValueOps::Multiply{}(left, right);
        } else if (op == TokenType::Slash) {
            return ValueOps::Divide{}(left, right);
        } else if (op == TokenType::Percent) {
            return ValueOps::Mod{}(left, right);
        } else if (op == TokenType::Equal) {
            return ValueOps::IsEqual{}(left, right);
        } else if (op == TokenType::NotEqual) {
            return ValueOps::IsNotEqual{}(left, right);
        } else if (op == TokenType::Less) {
            return ValueOps::IsLess{}(left, right);
        } else if (op == TokenType::Greater) {
            return ValueOps::IsGreater{}(left, right);
        } else if (op == TokenType::LessEqual) {
            return ValueOps::IsLessEqual{}(left, right);
        } else if (op == TokenType::GreaterEqual) {
            return ValueOps::IsGreaterEqual{}(left, right);
        } else if (op == TokenType::And) {
            return ValueOps::And{}(left, right);
        } else if (op == TokenType::Or) {
            return ValueOps::Or{}(left, right);
        }

        throw interpret_error("Unknown binary operation", position);

    }

};


class CallableExpr : public BaseExpr {
    std::shared_ptr<Function> func_;
    std::vector<ExprNode> args_val_;

public:
    CallableExpr(std::shared_ptr<Function> func, std::vector<ExprNode>&& args_val = std::vector<ExprNode>(), Pos position = Pos())
        : BaseExpr(position)
        , func_(std::move(func))
        , args_val_(std::move(args_val))
    {}

    Value evaluate(Environment& env) const override {
        return func_->call(args_val_, env);
    }

};


// Statements


class ExprStmt : public BaseStmt {
    ExprNode expression;

public:
    ExprStmt(ExprNode&& expr)
        : expression(std::move(expr))
    {}

    bool execute(Environment& env) override {
        auto res = expression->evaluate(env);

        return true;

    }
};

class SkipStmt : public BaseStmt {
public:
    bool execute(Environment& env) override {
        return true;
    }
};

class IfStmt : public BaseStmt {
public:
    IfStmt(ExprNode&& condition, std::vector<StmtNode>&& then_branch, std::vector<StmtNode>&& else_branch = std::vector<StmtNode>())
        : condition_(std::move(condition))
        , body_(std::move(then_branch))
        , else_body_(std::move(else_branch))
    {}

    // Returns true if the then-branch was taken
    bool execute(Environment& env) override {
        Value condition = condition_->evaluate(env);

        if (condition != 0) {
            for (auto& it : body_) {
                it->execute(env);
            }
            return true;
        }

        for (auto& it : else_body_) {
            it->execute(env);
        }

        return false;

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

    bool execute(Environment& env) override {

        while (condition_->evaluate(env) != 0) {
            for (auto& it : body_) {
                it->execute(env);
            }
        }

        return true;

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

    bool execute(Environment& env) override {

        do {
            for (auto& it : body_) {
                it->execute(env);
            }
        } while (condition_->evaluate(env) != 0);

        return true;

    }

private:
    ExprNode condition_;

    std::vector<StmtNode> body_;

};


// for (init cond; step) body
class ForStmt : public BaseStmt {
public:
    ForStmt(std::vector<StmtNode>&& init, ExprNode&& condition, std::vector<StmtNode>&& step, std::vector<StmtNode>&& body)
        : init_(std::move(init))
        , condition_(std::move(condition))
        , step_(std::move(step))
        , body_(std::move(body))
    {}

    bool execute(Environment& env) override {

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

        return true;

    }

private:
    std::vector<StmtNode> init_;
    ExprNode condition_;
    std::vector<StmtNode> step_;
    std::vector<StmtNode> body_;

};


class AST {
public:

    void push_back(StmtNode&& block) {
        blocks_.push_back(std::move(block));
    }

    void pop_front() {
        blocks_.pop_front();
    }

    bool empty() const {
        return blocks_.empty();
    }

    StmtNode& front() {
        return blocks_.front();
    }

    auto begin() {
        return blocks_.begin();
    }
    auto end() {
        return blocks_.end();
    }

private:
    std::deque<StmtNode> blocks_;

};

};
