#include "ast.h"
#include <limits>

namespace TomatoInterpretato {

namespace {

struct BinOpName {
    const char* text;
    BinOp op;
};

constexpr BinOpName kBinOps[] = {
    {"!!", BinOp::Or},
    {"&&", BinOp::And},
    {"==", BinOp::Equal},
    {"!=", BinOp::NotEqual},
    {"<=", BinOp::LessEqual},
    {"<", BinOp::Less},
    {">=", BinOp::GreaterEqual},
    {">", BinOp::Greater},
    {"+", BinOp::Plus},
    {"-", BinOp::Minus},
    {"*", BinOp::Star},
    {"/", BinOp::Slash},
    {"%", BinOp::Percent},
};

// Two's complement wraparound instead of signed overflow UB
Value wrap(std::uint64_t value) {
    return static_cast<Value>(value);
}

}  // namespace

bool parse_binop(const std::string& text, BinOp& op) {
    for (const auto& entry : kBinOps) {
        if (text == entry.text) {
            op = entry.op;
            return true;
        }
    }
    return false;
}

std::string binop_to_string(BinOp op) {
    for (const auto& entry : kBinOps) {
        if (entry.op == op) return entry.text;
    }
    return "?";
}

Value apply_binop(BinOp op, Value lhs, Value rhs, Pos position) {
    using U = std::uint64_t;

    switch (op) {
        case BinOp::Or:           return (lhs != 0 || rhs != 0) ? 1 : 0;
        case BinOp::And:          return (lhs != 0 && rhs != 0) ? 1 : 0;
        case BinOp::Equal:        return lhs == rhs ? 1 : 0;
        case BinOp::NotEqual:     return lhs != rhs ? 1 : 0;
        case BinOp::LessEqual:    return lhs <= rhs ? 1 : 0;
        case BinOp::Less:         return lhs < rhs ? 1 : 0;
        case BinOp::GreaterEqual: return lhs >= rhs ? 1 : 0;
        case BinOp::Greater:      return lhs > rhs ? 1 : 0;
        case BinOp::Plus:         return wrap(static_cast<U>(lhs) + static_cast<U>(rhs));
        case BinOp::Minus:        return wrap(static_cast<U>(lhs) - static_cast<U>(rhs));
        case BinOp::Star:         return wrap(static_cast<U>(lhs) * static_cast<U>(rhs));

        case BinOp::Slash:
        case BinOp::Percent:
            if (rhs == 0) {
                throw interpret_error("Division by zero", position);
            }
            if (lhs == std::numeric_limits<Value>::min() && rhs == -1) {
                return op == BinOp::Slash ? lhs : 0;
            }
            // truncation toward zero, remainder has the sign of the dividend
            return op == BinOp::Slash ? lhs / rhs : lhs % rhs;
    }

    throw interpret_error("Unknown binary operator", position);
}

};
