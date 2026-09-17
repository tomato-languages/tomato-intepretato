#pragma once
#include <cstdint>
#include <limits>
#include "errors.h"

namespace TomatoInterpretato {

using Value = std::int64_t;

namespace ValueOps {

inline Value Wrap(std::uint64_t value) {
    return static_cast<Value>(value);
}

struct Additive {
    Value operator()(Value lhs, Value rhs) const {
        return Wrap(static_cast<std::uint64_t>(lhs) + static_cast<std::uint64_t>(rhs));
    }
};

struct Substract {
    Value operator()(Value lhs, Value rhs) const {
        return Wrap(static_cast<std::uint64_t>(lhs) - static_cast<std::uint64_t>(rhs));
    }
};

struct Multiply {
    Value operator()(Value lhs, Value rhs) const {
        return Wrap(static_cast<std::uint64_t>(lhs) * static_cast<std::uint64_t>(rhs));
    }
};

struct Divide {
    Value operator()(Value lhs, Value rhs) const {
        if (rhs == 0) throw interpret_error("Division by zero");
        if (lhs == std::numeric_limits<Value>::min() && rhs == -1) return lhs;
        return lhs / rhs;
    }
};

struct Mod {
    Value operator()(Value lhs, Value rhs) const {
        if (rhs == 0) throw interpret_error("Division by zero");
        if (lhs == std::numeric_limits<Value>::min() && rhs == -1) return 0;
        return lhs % rhs;
    }
};

struct IsEqual {
    Value operator()(Value lhs, Value rhs) const { return lhs == rhs ? 1 : 0; }
};

struct IsNotEqual {
    Value operator()(Value lhs, Value rhs) const { return lhs != rhs ? 1 : 0; }
};

struct IsLess {
    Value operator()(Value lhs, Value rhs) const { return lhs < rhs ? 1 : 0; }
};

struct IsGreater {
    Value operator()(Value lhs, Value rhs) const { return lhs > rhs ? 1 : 0; }
};

struct IsLessEqual {
    Value operator()(Value lhs, Value rhs) const { return lhs <= rhs ? 1 : 0; }
};

struct IsGreaterEqual {
    Value operator()(Value lhs, Value rhs) const { return lhs >= rhs ? 1 : 0; }
};

struct And {
    Value operator()(Value lhs, Value rhs) const { return (lhs != 0 && rhs != 0) ? 1 : 0; }
};

struct Or {
    Value operator()(Value lhs, Value rhs) const { return (lhs != 0 || rhs != 0) ? 1 : 0; }
};

};

};
