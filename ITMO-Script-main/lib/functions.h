#pragma once
#include "ast.h"

namespace ItmoScript {

class PrintFunction : public Function {
    std::ostream& out_;
    
public:
    PrintFunction(std::ostream& out)
        : out_(out)
    {}

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {
        for (const auto& arg : args_val) {
            auto res = arg->evaluate(env);
            std::visit(ValueOps::Print{out_}, res);
        }
        return nullptr; 
    }

};
class PrintlnFunction : public Function {
    std::ostream& out_;
    
public:
    PrintlnFunction(std::ostream& out)
        : out_(out)
    {}

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {
        for (const auto& arg : args_val) {
            auto res = arg->evaluate(env);
            std::visit(ValueOps::Println{out_}, res);
        }
        return nullptr; 
    }

};

class LenFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Len{}, args_val[0]->evaluate(env));
    }

};

class AbsFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Abs{}, args_val[0]->evaluate(env));
    }

};

class RangeFunction : public Function {
    static constexpr size_t arity_ = 3;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() > arity_ || args_val.size() < 1) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        std::optional<double> start = 0;
        std::optional<double> end;
        std::optional<double> step = 1;

        for (auto i = 0; i < args_val.size(); ++i) {
            auto res = args_val[i]->evaluate(env);
            if (std::holds_alternative<double>(res)) {
                if (i == 0) {
                    end = std::get<double>(res);
                } else if (i == 1) {
                    start = std::get<double>(res);
                    std::swap(start, end);
                } else if (i == 2) {
                    step = std::get<double>(res);
                }
            } else {
                throw interpret_error("Range function arguments must be numbers", args_val[i]->position);
            }
        }

        std::shared_ptr<List> result_list = std::make_shared<List>();
        
        if (step.value() > 0) {
            for (int i = start.value(); i < end.value(); i += step.value()) {
                result_list->push_back(static_cast<double>(i));
            }   
        } else if (step.value() < 0) {
            for (int i = start.value(); i > end.value(); i += step.value()) {
                result_list->push_back(static_cast<double>(i));
            }
        } else {
            throw interpret_error("Range step cannot be zero", args_val[0]->position);
        }
        
        return result_list;
 
    }

};


class CeilFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Ceil{}, args_val[0]->evaluate(env));
    }

};

class FloorFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Floor{}, args_val[0]->evaluate(env));
    }

};

class RoundFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Round{}, args_val[0]->evaluate(env));
    }

};

class SqrtFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Sqrt{}, args_val[0]->evaluate(env));
    }

};

class RndFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Rnd{}, args_val[0]->evaluate(env));
    }

};

class ParseNumFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::ParseNum{}, args_val[0]->evaluate(env));
    }

};

class ToStringFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::ToString{}, args_val[0]->evaluate(env));
    }

};

class LowerFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Lower{}, args_val[0]->evaluate(env));
    }

};

class UpperFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Upper{}, args_val[0]->evaluate(env));
    }

};

class SplitFunction : public Function {
    static constexpr size_t arity_ = 2;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        if (!std::holds_alternative<std::shared_ptr<std::string>>(args_val[1]->evaluate(env))) {
            throw interpret_error("Delimiter for split must be a string", args_val[1]->position);
        }

        auto delimiter = std::get<std::shared_ptr<std::string>>(args_val[1]->evaluate(env));

        return std::visit(ValueOps::Split{*delimiter}, args_val[0]->evaluate(env));
    }

};

class JoinFunction : public Function {
    static constexpr size_t arity_ = 2;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        if (!std::holds_alternative<std::shared_ptr<std::string>>(args_val[1]->evaluate(env))) {
            throw interpret_error("Delimiter for join must be a string", args_val[1]->position);
        }

        auto delimiter = std::get<std::shared_ptr<std::string>>(args_val[1]->evaluate(env));

        return std::visit(ValueOps::Join{*delimiter}, args_val[0]->evaluate(env));
    }

};

class ReplaceFunction : public Function {
    static constexpr size_t arity_ = 3;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        if (!std::holds_alternative<std::shared_ptr<std::string>>(args_val[1]->evaluate(env)) || !std::holds_alternative<std::shared_ptr<std::string>>(args_val[2]->evaluate(env))) {
            throw interpret_error("Argument for replace must be a strings", args_val[1]->position);
        }

        auto old_substr = std::get<std::shared_ptr<std::string>>(args_val[1]->evaluate(env));
        auto new_substr = std::get<std::shared_ptr<std::string>>(args_val[2]->evaluate(env));

        return std::visit(ValueOps::Replace{*old_substr, *new_substr}, args_val[0]->evaluate(env));
    }

};

class PushFunction : public Function {
    static constexpr size_t arity_ = 2;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Push{args_val[1]->evaluate(env)}, args_val[0]->evaluate(env));
    }

};

class PopFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Pop{}, args_val[0]->evaluate(env));
    }

};
class InsertFunction : public Function {
    static constexpr size_t arity_ = 3;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        if (!std::holds_alternative<double>(args_val[1]->evaluate(env))) {
            throw interpret_error("Index for insert must be a number", args_val[1]->position);
        }

        int index = std::get<double>(args_val[1]->evaluate(env));

        if (index < 0) {
            throw interpret_error("Index for insert must be >= 0", args_val[1]->position);
        }

        return std::visit(ValueOps::Insert{index, args_val[2]->evaluate(env)}, args_val[0]->evaluate(env));
    }

};
class RemoveFunction : public Function {
    static constexpr size_t arity_ = 2;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        if (!std::holds_alternative<double>(args_val[1]->evaluate(env))) {
            throw interpret_error("Index for remove must be a number", args_val[1]->position);
        }

        int index = std::get<double>(args_val[1]->evaluate(env));

        if (index < 0) {
            throw interpret_error("Index for insert must be >= 0", args_val[1]->position);
        }

        return std::visit(ValueOps::Remove{index}, args_val[0]->evaluate(env));
    }

};

class SortFunction : public Function {
    static constexpr size_t arity_ = 1;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        return std::visit(ValueOps::Sort{}, args_val[0]->evaluate(env));
    }

};

class ReadFunction : public Function {
    static constexpr size_t arity_ = 0;
public:

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {

        if (args_val.size() != arity_) {
            throw interpret_error("Callable arguments count mismatch", args_val[0]->position);
        }

        std::string input;
        std::getline(std::cin, input);

        return new_string(std::move(input));
    }

};


};
