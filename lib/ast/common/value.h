#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include "common/errors.h"

namespace NTomatoInterpretato {

using Value = std::int64_t;

namespace ValueOps {

inline bool IsOperator(const std::string& op) {
    return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
        op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" ||
        op == ">=" || op == "&&" || op == "!!";
}

inline Value Apply(const std::string& op, Value lhs, Value rhs) {
    auto left = static_cast<std::uint64_t>(lhs);
    auto right = static_cast<std::uint64_t>(rhs);
    if (op == "+") return static_cast<Value>(left + right);
    if (op == "-") return static_cast<Value>(left - right);
    if (op == "*") return static_cast<Value>(left * right);
    if (op == "/" || op == "%") {
        if (rhs == 0) throw interpret_error("Division by zero");
        if (lhs == std::numeric_limits<Value>::min() && rhs == -1) return op == "/" ? lhs : 0;
        return op == "/" ? lhs / rhs : lhs % rhs;
    }
    if (op == "==") return lhs == rhs;
    if (op == "!=") return lhs != rhs;
    if (op == "<") return lhs < rhs;
    if (op == ">") return lhs > rhs;
    if (op == "<=") return lhs <= rhs;
    if (op == ">=") return lhs >= rhs;
    if (op == "&&") return lhs != 0 && rhs != 0;
    if (op == "!!") return lhs != 0 || rhs != 0;
    throw interpret_error("Unknown binary operation: " + op);
}

}
}
