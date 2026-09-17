#pragma once
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include "lexer.h"
#include "value.cpp"
#include "errors.h"

namespace ItmoScript {

using Environment = std::unordered_map<std::string, Value>;

struct return_exception : public interpret_error {
    Value value;

    explicit return_exception(Value val, Pos position = Pos())
        : interpret_error("Return keyword must be in function", position)
        , value(std::move(val))
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


struct KV {
    ExprNode key;
    ExprNode value;
};


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
        : lhs(std::move(lhs)), rhs(std::move(rhs))
        , BaseExpr(position)
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

    double value;
public:
    NumberExpr(double val, Pos position = Pos())
        : value(val)
        , BaseExpr(position)
    {};

    Value evaluate(Environment& env) const override {
        return value;
    }

};
class StringExpr : public BaseExpr {

    std::string value;

public:
    StringExpr(const std::string& val, Pos position)
        : BaseExpr(position)
        , value(val)
    {};
    
    Value evaluate(Environment& env) const override {
        return std::make_shared<std::string>(value);
    }

};

class NilExpr : public BaseExpr {
public:
    NilExpr(Pos position = Pos())
        : BaseExpr(position)
    {}

    Value evaluate(Environment& env) const override {
        return nullptr;
    }
};


class Function {
private:
    std::vector<ExprNode> args_;
    std::vector<StmtNode> body_;
    
public:
    Function(std::vector<ExprNode>&& args = std::vector<ExprNode>(), std::vector<StmtNode>&& body = std::vector<StmtNode>())
        : args_(std::move(args))
        , body_(std::move(body))
    {}

    size_t arity() const {
        return args_.size();
    }

    virtual Value call(const std::vector<ExprNode>& args_val, Environment& env) const {

        Environment local_env = env;

        if (args_val.size() != args_.size()) {
            throw interpret_error("Callable arguments count mismatch", args_[0]->position);
        }


        for (size_t i = 0; i < args_.size(); ++i) {
            if (auto var_node = dynamic_cast<VariableExpr*>(args_[i].get())) {
                local_env[var_node->var_name] = args_val[i]->evaluate(env);
            } else {
                throw interpret_error("Callable argument must be a variable", args_[i]->position);
            }
        }

        try {
            for (const auto& stmt : body_) {
                stmt->execute(local_env);
            }
        } catch (const return_exception& e) {
            return e.value;
        }

        return nullptr; 
    }

};

class FunctionExpr : public BaseExpr {
    std::shared_ptr<Function> func_;

public:
    FunctionExpr(std::vector<ExprNode>&& args = std::vector<ExprNode>(), std::vector<StmtNode>&& body = std::vector<StmtNode>(), Pos position = Pos())
        : BaseExpr(position)
        , func_(std::make_shared<Function>(std::move(args), std::move(body)))
    {}

    Value evaluate(Environment& env) const override {
        return func_;
    }

};


class ListExpr : public BaseExpr {
    std::vector<ExprNode> elements_;

public:
    ListExpr(std::vector<ExprNode>&& elements, Pos position = Pos())
        : BaseExpr(position)
        , elements_(std::move(elements))
    {}

    Value evaluate(Environment& env) const override {
        std::shared_ptr<List> list = std::make_shared<List>();
        
        for (auto& i : elements_) {
            list->push_back(i->evaluate(env));
        }

        return list;

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
        Dictionary dict;
        
        for (auto& i : elements_) {
            dict.insert({i.key->evaluate(env), i.value->evaluate(env)});
        }

        return std::make_shared<Dictionary>(dict);

    }


};

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
            return std::visit(ValueOps::Additive{}, left, right);
        } else if (op == TokenType::Minus) {
            return std::visit(ValueOps::Substract{}, left, right);
        } else if (op == TokenType::Star) {
            return std::visit(ValueOps::Multiply{}, left, right);
        } else if (op == TokenType::Slash) {
            return std::visit(ValueOps::Divide{}, left, right);
        } else if (op == TokenType::Percent) {
            return std::visit(ValueOps::Mod{}, left, right);
        } else if (op == TokenType::Caret) {
            return std::visit(ValueOps::Power{}, left, right);
        } else if (op == TokenType::Equal) {
            return std::visit(ValueOps::IsEqual{}, left, right);
        } else if (op == TokenType::NotEqual) {
            return std::visit(ValueOps::IsNotEqual{}, left, right);
        } else if (op == TokenType::Less) {
            return std::visit(ValueOps::IsLess{}, left, right);
        } else if (op == TokenType::Greater) {
            return std::visit(ValueOps::IsGreater{}, left, right);
        } else if (op == TokenType::LessEqual) {
            return std::visit(ValueOps::IsLessEqual{}, left, right);
        } else if (op == TokenType::GreaterEqual) {
            return std::visit(ValueOps::IsGreaterEqual{}, left, right);
        } else if (op == TokenType::And) {
            return std::visit(ValueOps::And{}, left, right);
        } else if (op == TokenType::Or) {
            return std::visit(ValueOps::Or{}, left, right);
        } else {
            throw interpret_error("Unknown binary operation");
        }

        throw interpret_error("Unknown binary operation");

    }

};
class UnaryOpExpr : public BaseExpr{
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
            return std::visit(ValueOps::Revert{}, right);
        } else if (op == TokenType::Plus) {
            return right; 
        } else if (op == TokenType::Not) {
            return std::visit(ValueOps::Not{}, right);
        } else { 
            throw interpret_error("Unknown unary operation");
        }

    }

};


class CallableExpr : public BaseExpr {
    ExprNode func_expr_;
    std::vector<ExprNode> args_val_;

public:
    CallableExpr(ExprNode&& func, std::vector<ExprNode>&& args_val = std::vector<ExprNode>(), Pos position = Pos())
        : BaseExpr(position)
        , func_expr_(std::move(func))
        , args_val_(std::move(args_val))
    {}

    Value evaluate(Environment& env) const override {

        Value func_val = func_expr_->evaluate(env);

        if (!std::holds_alternative<std::shared_ptr<Function>>(func_val)) {
            throw interpret_error("Callable expression must be a function", func_expr_->position);
        }

        auto func = std::get<std::shared_ptr<Function>>(func_val);

        return func->call(args_val_, env);      
    }

};

class IndexExpr : public BaseExpr {
    ExprNode collection_expr_;
    ExprNode index_expr_;

public:
    IndexExpr(ExprNode&& collection_expr, ExprNode&& index_expr, Pos position = Pos())
        : BaseExpr(position)
        , collection_expr_(std::move(collection_expr))
        , index_expr_(std::move(index_expr))

    {}

    Value evaluate(Environment& env) const override {

        Value collection = collection_expr_->evaluate(env);
        Value index_val = index_expr_->evaluate(env);

        return std::visit(ValueOps::Index{index_val}, collection);
    }

};

class SliceExpr : public BaseExpr {
    ExprNode collection_expr_;
    std::optional<ExprNode> start_;
    std::optional<ExprNode> end_;
    std::optional<ExprNode> step_;
    

public:
    SliceExpr(ExprNode&& collection_expr, std::optional<ExprNode>&& start, std::optional<ExprNode>&& end, std::optional<ExprNode>&& step, Pos position = Pos())
        : BaseExpr(position)
        , collection_expr_(std::move(collection_expr))
        , start_(std::move(start))
        , end_(std::move(end))
        , step_(std::move(step))
    {}

    Value evaluate(Environment& env) const override {
        
        Value collection = collection_expr_->evaluate(env);
 

        if (!std::holds_alternative<std::shared_ptr<List>>(collection) && !std::holds_alternative<std::shared_ptr<std::string>>(collection)) {
            throw interpret_error("Slice expression must be a indexable type (list or string)", collection_expr_->position);
        }

        double len = std::get<double>(std::visit(ValueOps::Len{}, collection));

        std::optional<Value> start_val = start_ ? std::optional<Value>(start_.value()->evaluate(env)) : std::nullopt;
        std::optional<Value> end_val = end_ ? std::optional<Value>(end_.value()->evaluate(env)) : std::nullopt;
        std::optional<Value> step_val = step_ ? std::optional<Value>(step_.value()->evaluate(env)) : std::nullopt;

        if (start_val) if (!std::holds_alternative<double>(start_val.value())) throw interpret_error("Slice start must be a value");
        if (end_val) if (!std::holds_alternative<double>(end_val.value())) throw interpret_error("Slice end must be a value");
        if (step_val) if (!std::holds_alternative<double>(step_val.value())) throw interpret_error("Slice step must be a value");
    
        std::optional<int> start_opt = start_val ? std::optional<int>(static_cast<int>(std::get<double>(start_val.value()))) : std::nullopt;
        std::optional<int> end_opt = end_val ? std::optional<int>(static_cast<int>(std::get<double>(end_val.value()))) : std::nullopt;
        std::optional<int> step_opt = step_val ? std::optional<int>(static_cast<int>(std::get<double>(step_val.value()))) : std::nullopt;

        return std::visit(ValueOps::Slice{start_opt, end_opt, step_opt}, collection);
      
    }

};

class ReturnStmt : public BaseStmt {
    
    ExprNode return_value_;

public:

    ReturnStmt(ExprNode&& return_value)
        : return_value_(std::move(return_value))
    {}

    bool execute(Environment& env) override {
        if (return_value_) {
            throw return_exception(return_value_->evaluate(env));
        } else {
            throw return_exception(nullptr);
        }

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

class IfStmt : public BaseStmt {
public:
    IfStmt(ExprNode&& condition, std::vector<StmtNode>&& then_branch, std::vector<StmtNode>&& else_branch = std::vector<StmtNode>())
        : condition_(std::move(condition))
        , body_(std::move(then_branch))
        , else_body_(std::move(else_branch))
    {}

    bool execute(Environment& env) override {
        Value condition = condition_->evaluate(env);

        if (std::holds_alternative<double>(condition)) {

            if (std::get<double>(condition) != 0) {
                for (auto& it : body_) {
                    it->execute(env);
                }
            } else {

                for (auto& it : else_body_) {
                    if (it->execute(env)) break;
                }

                return false;

            }

        } else {
            throw interpret_error("If condition must be a bool", condition_->position);
        }

        return true;


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
        Value condition = condition_->evaluate(env);

        if (std::holds_alternative<double>(condition)) {
            while (std::get<double>(condition_->evaluate(env)) != 0) {
                try {
                    for (auto& it : body_) {
                        it->execute(env);
                    }
                    condition = condition_->evaluate(env);
                } catch (const break_exception&) {
                    break;
                } catch (const continue_exception&) {
                    continue;
                }
            } 
            for (auto& it : else_body_) {
                if (it->execute(env)) break;
            }


        } else {
            throw interpret_error("While condition must be a value", condition_->position);
        }

        return true;


    }

private:
    ExprNode condition_;
    
    std::vector<StmtNode> body_;
    std::vector<StmtNode> else_body_;

};


class ForStmt : public BaseStmt {
public:
    ForStmt(ExprNode&& collection_expr, std::vector<StmtNode>&& body, const std::string& index_var_name)
        : collection_expr_(std::move(collection_expr))
        , body_(std::move(body))
        , index_var_name_(index_var_name)

    {}

    bool execute(Environment& env) override {

        Value blackout_var_val;

        bool is_blackout = false;
        if(env.contains(index_var_name_)) {
            is_blackout = true;
            blackout_var_val = env[index_var_name_];
        }

        Value collection = collection_expr_->evaluate(env);

        if (!std::holds_alternative<std::shared_ptr<List>>(collection) && !std::holds_alternative<std::shared_ptr<std::string>>(collection)) {
            throw interpret_error("For loop collection must be a indexable type (list or string)", collection_expr_->position);
        }


        double len = std::get<double>(std::visit(ValueOps::Len{}, collection));

        for (double i = 0; i < len; i++) {

            env[index_var_name_] = std::visit(ValueOps::Index{Value(i)}, collection);

            for (auto& stmt : body_) {
                try {
                    stmt->execute(env);
                } catch (const break_exception&) {
                    goto EndFor;
                } catch (const continue_exception&) {
                    continue;
                }
            }

        }

        EndFor:

        if(is_blackout) {
            env[index_var_name_] = blackout_var_val;
        } else {
            env.erase(index_var_name_);
        }

        return true;


    }

private:
    ExprNode collection_expr_;
    
    std::string index_var_name_;
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
