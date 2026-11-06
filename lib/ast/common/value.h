#pragma once
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "common/errors.h"
#include "common/limits.h"

namespace NTomatoInterpretato {

using Integer = std::int64_t;

struct ArrayData;
using StringRef = std::shared_ptr<std::string>;
using ArrayRef = std::shared_ptr<ArrayData>;

struct Value {
    std::variant<Integer, StringRef, ArrayRef> data;

    Value() : data(Integer{0}) {}
    Value(Integer number) : data(number) {}
    Value(StringRef text) : data(std::move(text)) {}
    Value(ArrayRef array) : data(std::move(array)) {}
};

struct ArrayData {
    std::vector<Value> elements;

    ArrayData() = default;
    ArrayData(const ArrayData&) = delete;
    ArrayData& operator=(const ArrayData&) = delete;

    ~ArrayData() {
        std::vector<Value> pending = std::move(elements);
        while (!pending.empty()) {
            Value current = std::move(pending.back());
            pending.pop_back();
            auto* array = std::get_if<ArrayRef>(&current.data);
            if (array != nullptr && array->use_count() == 1) {
                auto& nested = (*array)->elements;
                pending.insert(pending.end(), std::make_move_iterator(nested.begin()),
                    std::make_move_iterator(nested.end()));
                nested.clear();
            }
        }
    }
};

namespace ValueOps {

inline bool IsOperator(const std::string& op) {
    return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
        op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" ||
        op == ">=" || op == "&&" || op == "!!";
}

inline Integer AsInteger(const Value& value) {
    const auto* number = std::get_if<Integer>(&value.data);
    if (number == nullptr) throw interpret_error("Integer value expected");
    return *number;
}

inline bool IsTrue(const Value& value) {
    return AsInteger(value) != 0;
}

using ComparePath = std::vector<std::pair<const ArrayData*, const ArrayData*>>;

inline int CompareValues(const Value& lhs, const Value& rhs, const std::string& op, ComparePath& path) {
    if (lhs.data.index() != rhs.data.index()) {
        throw interpret_error("Binary operator '" + op + "' is applied to values of different types");
    }
    if (const auto* left = std::get_if<Integer>(&lhs.data)) {
        const Integer right = std::get<Integer>(rhs.data);
        return *left < right ? -1 : (*left > right ? 1 : 0);
    }
    if (const auto* left = std::get_if<StringRef>(&lhs.data)) {
        const int order = (*left)->compare(*std::get<StringRef>(rhs.data));
        return order < 0 ? -1 : (order > 0 ? 1 : 0);
    }
    const ArrayData* left = std::get<ArrayRef>(lhs.data).get();
    const ArrayData* right = std::get<ArrayRef>(rhs.data).get();
    if (left->elements.size() != right->elements.size()) {
        return left->elements.size() < right->elements.size() ? -1 : 1;
    }
    for (const auto& visited : path) {
        if (visited.first == left && visited.second == right) {
            throw interpret_error("Cannot compare arrays that contain themselves");
        }
    }
    if (path.size() >= static_cast<std::size_t>(kMaxNestingDepth)) {
        throw interpret_error("Arrays are nested too deeply to compare");
    }
    path.emplace_back(left, right);
    for (std::size_t i = 0; i < left->elements.size(); ++i) {
        const int order = CompareValues(left->elements[i], right->elements[i], op, path);
        if (order != 0) {
            path.pop_back();
            return order;
        }
    }
    path.pop_back();
    return 0;
}

inline Integer Arithmetic(const std::string& op, Integer lhs, Integer rhs) {
    auto left = static_cast<std::uint64_t>(lhs);
    auto right = static_cast<std::uint64_t>(rhs);
    if (op == "+") return static_cast<Integer>(left + right);
    if (op == "-") return static_cast<Integer>(left - right);
    if (op == "*") return static_cast<Integer>(left * right);
    if (op == "/" || op == "%") {
        if (rhs == 0) throw interpret_error("Division by zero");
        if (lhs == std::numeric_limits<Integer>::min() && rhs == -1) return op == "/" ? lhs : 0;
        return op == "/" ? lhs / rhs : lhs % rhs;
    }
    if (op == "&&") return lhs != 0 && rhs != 0;
    if (op == "!!") return lhs != 0 || rhs != 0;
    throw interpret_error("Unknown binary operation: " + op);
}

inline Value Apply(const std::string& op, const Value& lhs, const Value& rhs) {
    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
        ComparePath path;
        const int order = CompareValues(lhs, rhs, op, path);
        if (op == "==") return Integer{order == 0};
        if (op == "!=") return Integer{order != 0};
        if (op == "<") return Integer{order < 0};
        if (op == ">") return Integer{order > 0};
        if (op == "<=") return Integer{order <= 0};
        return Integer{order >= 0};
    }
    return Arithmetic(op, AsInteger(lhs), AsInteger(rhs));
}

inline Integer Length(const Value& value) {
    if (const auto* text = std::get_if<StringRef>(&value.data)) return static_cast<Integer>((*text)->size());
    if (const auto* array = std::get_if<ArrayRef>(&value.data)) return static_cast<Integer>((*array)->elements.size());
    throw interpret_error("Array or string value expected");
}

inline std::size_t CheckedIndex(const Value& container, const Value& index) {
    const Integer size = Length(container);
    const Integer position = AsInteger(index);
    if (position < 0 || position >= size) {
        throw interpret_error("Index " + std::to_string(position) + " is out of bounds [0, " + std::to_string(size) + ")");
    }
    return static_cast<std::size_t>(position);
}

inline Value Elem(const Value& container, const Value& index) {
    const std::size_t position = CheckedIndex(container, index);
    if (const auto* text = std::get_if<StringRef>(&container.data)) {
        return Integer{static_cast<unsigned char>((**text)[position])};
    }
    return std::get<ArrayRef>(container.data)->elements[position];
}

inline void SetElem(const Value& container, const Value& index, const Value& value) {
    const std::size_t position = CheckedIndex(container, index);
    if (const auto* text = std::get_if<StringRef>(&container.data)) {
        const Integer code = AsInteger(value);
        if (code < 0 || code > 255) {
            throw interpret_error("Character code " + std::to_string(code) + " is out of range [0, 255]");
        }
        (**text)[position] = static_cast<char>(static_cast<unsigned char>(code));
        return;
    }
    std::get<ArrayRef>(container.data)->elements[position] = value;
}

inline void Format(const Value& value, std::string& out, std::vector<const ArrayData*>& path) {
    if (const auto* number = std::get_if<Integer>(&value.data)) {
        out += std::to_string(*number);
        return;
    }
    if (const auto* text = std::get_if<StringRef>(&value.data)) {
        if (!path.empty()) out += '"';
        out += **text;
        if (!path.empty()) out += '"';
        return;
    }
    const ArrayData* array = std::get<ArrayRef>(value.data).get();
    for (const ArrayData* visited : path) {
        if (visited == array) throw interpret_error("Cannot print an array that contains itself");
    }
    if (path.size() >= static_cast<std::size_t>(kMaxNestingDepth)) {
        throw interpret_error("Array is nested too deeply to print");
    }
    path.push_back(array);
    out += '[';
    for (std::size_t i = 0; i < array->elements.size(); ++i) {
        if (i != 0) out += ", ";
        Format(array->elements[i], out, path);
    }
    out += ']';
    path.pop_back();
}

inline void Write(std::ostream& output, const Value& value) {
    std::string text;
    std::vector<const ArrayData*> path;
    Format(value, text, path);
    output << text << '\n';
}

}
}
