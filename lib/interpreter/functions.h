#pragma once
#include "ast/ast.h"
#include "common/errors.h"

namespace NTomatoInterpretato {

using NAst::Function;
using NAst::ExprNode;
using NAst::Environment;
using NAst::VariableExpr;

class WriteFunction : public Function {
    std::ostream& out_;
    static constexpr size_t arity_ = 1;

public:
    WriteFunction(std::ostream& out)
        : out_(out)
    {}

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {
        if (args_val.size() != arity_) {
            throw interpret_error("write: callable arguments count mismatch");
        }

        Value res = args_val[0]->evaluate(env);
        out_ << res << '\n';

        return res;
    }

};

class ReadFunction : public Function {
    std::istream& in_;
    static constexpr size_t arity_ = 1;

public:
    ReadFunction(std::istream& in)
        : in_(in)
    {}

    Value call(const std::vector<ExprNode>& args_val, Environment& env) const override {
        if (args_val.size() != arity_) {
            throw interpret_error("read: callable arguments count mismatch");
        }

        auto var_node = dynamic_cast<VariableExpr*>(args_val[0].get());
        if (!var_node) {
            throw interpret_error("read: argument must be a variable", args_val[0]->position);
        }

        Value input;
        if (!(in_ >> input)) {
            throw interpret_error("read(" + var_node->var_name + "): expected an integer on input", var_node->position);
        }

        return env[var_node->var_name] = input;
    }

};

}
