#pragma once
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <optional>
#include "ast/common/value.h"
#include "common/errors.h"

namespace NTomatoInterpretato {
namespace NAst {

enum class TokenType {
    Read, Write,
    If, Then, Elif, ElseIf, Else,
    While, Do, For, In,
    Break, Continue, End, Skip,

    Function, Return,
    True, False, Nil,

    Identifier,

    And,
    Or,
    Not,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,

    Assign,
    OrAssign,
    AndAssign,
    PlusAssign,
    MinusAssign,
    StarAssign,
    SlashAssign,
    PercentAssign,
    CaretAssign,

    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,

    Comma,
    Colon,
    Semicolon,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,

    Number,
    String,

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

class StringExpr : public BaseExpr {
    std::string value_;
public:
    StringExpr(const std::string& val, Pos position)
        : BaseExpr(position)
        , value_(val)
    {}

    Value evaluate(Environment& env) const override {
        throw interpret_error("String values are not supported", position);
    }
};

class NilExpr : public BaseExpr {
public:
    NilExpr(Pos position = Pos())
        : BaseExpr(position)
    {}

    Value evaluate(Environment& env) const override {
        throw interpret_error("Nil values are not supported", position);
    }
};

struct KV {
    ExprNode key;
    ExprNode value;
};

class ListExpr : public BaseExpr {
    std::vector<ExprNode> elements_;
public:
    ListExpr(std::vector<ExprNode>&& elements, Pos position = Pos())
        : BaseExpr(position)
        , elements_(std::move(elements))
    {}

    Value evaluate(Environment& env) const override {
        throw interpret_error("List values are not supported", position);
    }
};

class DictionaryExpr : public BaseExpr {
    std::vector<KV> elements_;
public:
    DictionaryExpr(std::vector<KV>&& elements, Pos position = Pos())
        : BaseExpr(position)
        , elements_(std::move(elements))
    {}

    Value evaluate(Environment& env) const override {
        throw interpret_error("Dictionary values are not supported", position);
    }
};

class FunctionExpr : public BaseExpr {
public:
    FunctionExpr(std::vector<ExprNode>&&, std::vector<StmtNode>&&, Pos position = Pos())
        : BaseExpr(position)
    {}

    Value evaluate(Environment& env) const override {
        throw interpret_error("User-defined functions are not supported", position);
    }
};

class UnaryOpExpr : public BaseExpr {
    TokenType op;
    ExprNode rhs;
public:
    UnaryOpExpr(TokenType op, ExprNode&& rhs, Pos position)
        : BaseExpr(position)
        , op(op)
        , rhs(std::move(rhs))
    {}

    Value evaluate(Environment& env) const override {
        Value right = rhs->evaluate(env);
        if (op == TokenType::Minus) {
            return -right;
        }
        if (op == TokenType::Plus) {
            return right;
        }
        if (op == TokenType::Not) {
            return right == 0 ? 1 : 0;
        }
        throw interpret_error("Unknown unary operation", position);
    }
};

class IndexExpr : public BaseExpr {
public:
    IndexExpr(ExprNode&&, ExprNode&&, Pos position = Pos())
        : BaseExpr(position)
    {}

    Value evaluate(Environment& env) const override {
        throw interpret_error("Indexing is not supported", position);
    }
};

class SliceExpr : public BaseExpr {
public:
    SliceExpr(ExprNode&&, std::optional<ExprNode>&&, std::optional<ExprNode>&&, std::optional<ExprNode>&&, Pos position = Pos())
        : BaseExpr(position)
    {}

    Value evaluate(Environment& env) const override {
        throw interpret_error("Slicing is not supported", position);
    }
};

struct return_exception : public interpret_error {
    Value value;
    explicit return_exception(Value val, Pos position = Pos())
        : interpret_error("Return keyword must be in function", position)
        , value(val)
    {}
};

struct break_exception : public interpret_error {
    break_exception(Pos position = Pos())
        : interpret_error("Break keyword must be in while/for loop", position)
    {}
};

struct continue_exception : public interpret_error {
    continue_exception(Pos position = Pos())
        : interpret_error("Continue keyword must be in while/for loop", position)
    {}
};

class ReturnStmt : public BaseStmt {
    ExprNode return_value_;
public:
    ReturnStmt(ExprNode&& return_value)
        : return_value_(std::move(return_value))
    {}

    bool execute(Environment& env) override {
        throw return_exception(return_value_ ? return_value_->evaluate(env) : 0);
    }
};

class BreakStmt : public BaseStmt {
public:
    bool execute(Environment& env) override {
        throw break_exception();
    }
};

class ContinueStmt : public BaseStmt {
public:
    bool execute(Environment& env) override {
        throw continue_exception();
    }
};


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
    ExprNode func_expr_;
    std::vector<ExprNode> args_val_;

public:
    CallableExpr(std::shared_ptr<Function> func, std::vector<ExprNode>&& args_val = std::vector<ExprNode>(), Pos position = Pos())
        : BaseExpr(position)
        , func_(std::move(func))
        , args_val_(std::move(args_val))
    {}

    CallableExpr(ExprNode&& func, std::vector<ExprNode>&& args_val = std::vector<ExprNode>(), Pos position = Pos())
        : BaseExpr(position)
        , func_expr_(std::move(func))
        , args_val_(std::move(args_val))
    {}

    Value evaluate(Environment& env) const override {
        if (func_) {
            return func_->call(args_val_, env);
        }
        throw interpret_error("Callable expression must be a function", position);
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
    WhileStmt(ExprNode&& condition, std::vector<StmtNode>&& body, std::vector<StmtNode>&& else_body = std::vector<StmtNode>())
        : condition_(std::move(condition))
        , body_(std::move(body))
        , else_body_(std::move(else_body))
    {}

    bool execute(Environment& env) override {

        while (condition_->evaluate(env) != 0) {
            try {
                for (auto& it : body_) {
                    it->execute(env);
                }
            } catch (const break_exception&) {
                break;
            } catch (const continue_exception&) {
                continue;
            }
        }

        for (auto& it : else_body_) {
            it->execute(env);
        }

        return true;

    }

private:
    ExprNode condition_;

    std::vector<StmtNode> body_;
    std::vector<StmtNode> else_body_;

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


class ForStmt : public BaseStmt {
public:
    ForStmt(std::vector<StmtNode>&& init, ExprNode&& condition, std::vector<StmtNode>&& step, std::vector<StmtNode>&& body)
        : init_(std::move(init))
        , condition_(std::move(condition))
        , step_(std::move(step))
        , body_(std::move(body))
    {}

    ForStmt(ExprNode&& collection, std::vector<StmtNode>&& body, std::string index_var)
        : collection_(std::move(collection))
        , body_(std::move(body))
        , index_var_(std::move(index_var))
        , is_foreach_(true)
    {}

    bool execute(Environment& env) override {
        if (is_foreach_) {
            throw interpret_error("for-in loops are not supported");
        }

        for (auto& it : init_) {
            it->execute(env);
        }

        while (condition_->evaluate(env) != 0) {
            try {
                for (auto& it : body_) {
                    it->execute(env);
                }
            } catch (const break_exception&) {
                break;
            } catch (const continue_exception&) {
                for (auto& it : step_) {
                    it->execute(env);
                }
                continue;
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

    ExprNode collection_;
    std::string index_var_;
    bool is_foreach_ = false;

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

}
}
