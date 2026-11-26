#pragma once
#include <cctype>
#include <charconv>
#include <cstdint>
#include <istream>
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
struct SexpData;
using StringRef = std::shared_ptr<std::string>;
using ArrayRef = std::shared_ptr<ArrayData>;
using SexpRef = std::shared_ptr<SexpData>;

struct Value {
    std::variant<Integer, StringRef, ArrayRef, SexpRef> data;

    Value() : data(Integer{0}) {}
    Value(Integer number) : data(number) {}
    Value(StringRef text) : data(std::move(text)) {}
    Value(ArrayRef array) : data(std::move(array)) {}
    Value(SexpRef sexp) : data(std::move(sexp)) {}
};

struct SexpData {
    std::string tag;
    std::vector<Value> elements;
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
    if (const auto* left_sexp = std::get_if<SexpRef>(&lhs.data)) {
        const auto& right_sexp = std::get<SexpRef>(rhs.data);
        const int tag_order = (*left_sexp)->tag.compare(right_sexp->tag);
        if (tag_order != 0) return tag_order < 0 ? -1 : 1;
        if ((*left_sexp)->elements.size() != right_sexp->elements.size()) {
            return (*left_sexp)->elements.size() < right_sexp->elements.size() ? -1 : 1;
        }
        for (std::size_t i = 0; i < (*left_sexp)->elements.size(); ++i) {
            const int order = CompareValues((*left_sexp)->elements[i], right_sexp->elements[i], op, path);
            if (order != 0) return order;
        }
        return 0;
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

inline std::size_t SexpLength(const SexpData& sexp) {
    if (sexp.tag != "cons" && sexp.tag != "nil") return sexp.elements.size();
    std::size_t length = 0;
    const SexpData* current = &sexp;
    while (current->tag == "cons") {
        if (current->elements.empty() || current->elements.size() > 2) {
            throw interpret_error("Malformed cons S-expression");
        }
        ++length;
        if (current->elements.size() == 1) break;
        const auto* tail = std::get_if<SexpRef>(&current->elements[1].data);
        if (tail == nullptr) throw interpret_error("Improper cons S-expression");
        current = tail->get();
        if (current->tag != "cons" && current->tag != "nil") {
            throw interpret_error("Improper cons S-expression");
        }
    }
    return length;
}

inline Integer Length(const Value& value) {
    if (const auto* text = std::get_if<StringRef>(&value.data)) return static_cast<Integer>((*text)->size());
    if (const auto* array = std::get_if<ArrayRef>(&value.data)) return static_cast<Integer>((*array)->elements.size());
    if (const auto* sexp = std::get_if<SexpRef>(&value.data)) return static_cast<Integer>(SexpLength(**sexp));
    throw interpret_error("Array, string or S-expression value expected");
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
    if (const auto* sexp = std::get_if<SexpRef>(&container.data)) {
        const auto& value = **sexp;
        if (value.tag != "cons" && value.tag != "nil") return value.elements[position];
        const SexpData* current = &value;
        std::size_t at = 0;
        while (current->tag == "cons") {
            if (current->elements.empty() || current->elements.size() > 2) {
                throw interpret_error("Malformed cons S-expression");
            }
            if (at++ == position) return current->elements[0];
            if (current->elements.size() == 1) break;
            current = std::get<SexpRef>(current->elements[1].data).get();
        }
        throw interpret_error("Malformed cons S-expression");
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
    if (std::holds_alternative<SexpRef>(container.data)) {
        throw interpret_error("Cannot assign to an S-expression element");
    }
    std::get<ArrayRef>(container.data)->elements[position] = value;
}

inline void Format(const Value& value, std::string& out, std::vector<const ArrayData*>& path,
                   std::vector<const SexpData*>& sexp_path) {
    if (const auto* number = std::get_if<Integer>(&value.data)) {
        out += std::to_string(*number);
        return;
    }
    if (const auto* text = std::get_if<StringRef>(&value.data)) {
        if (!path.empty() || !sexp_path.empty()) out += '"';
        out += **text;
        if (!path.empty() || !sexp_path.empty()) out += '"';
        return;
    }
    if (const auto* sexp_ref = std::get_if<SexpRef>(&value.data)) {
        const SexpData* sexp = sexp_ref->get();
        for (const SexpData* visited : sexp_path) {
            if (visited == sexp) throw interpret_error("Cannot print a cyclic S-expression");
        }
        if (sexp_path.size() >= static_cast<std::size_t>(kMaxNestingDepth)) {
            throw interpret_error("S-expression is nested too deeply to print");
        }
        sexp_path.push_back(sexp);
        if (sexp->tag == "list" || sexp->tag == "nil") {
            out += '{';
            for (std::size_t i = 0; i < sexp->elements.size(); ++i) {
                if (i != 0) out += ", ";
                Format(sexp->elements[i], out, path, sexp_path);
            }
            out += '}';
        } else if (sexp->tag == "cons") {
            out += '{';
            const SexpData* current = sexp;
            bool first = true;
            while (current->tag == "cons") {
                if (current->elements.empty() || current->elements.size() > 2) {
                    throw interpret_error("Malformed cons S-expression");
                }
                if (!first) out += ", ";
                Format(current->elements[0], out, path, sexp_path);
                first = false;
                if (current->elements.size() == 1) break;
                const auto* tail = std::get_if<SexpRef>(&current->elements[1].data);
                if (tail == nullptr || (tail->get()->tag != "cons" && tail->get()->tag != "nil")) {
                    throw interpret_error("Improper cons S-expression");
                }
                current = tail->get();
            }
            out += '}';
        } else {
            out += sexp->tag;
            out += '(';
            for (std::size_t i = 0; i < sexp->elements.size(); ++i) {
                if (i != 0) out += ", ";
                Format(sexp->elements[i], out, path, sexp_path);
            }
            out += ')';
        }
        sexp_path.pop_back();
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
        Format(array->elements[i], out, path, sexp_path);
    }
    out += ']';
    path.pop_back();
}

inline void Write(std::ostream& output, const Value& value) {
    std::string text;
    std::vector<const ArrayData*> path;
    std::vector<const SexpData*> sexp_path;
    Format(value, text, path, sexp_path);
    output << text << '\n';
}

inline void SkipSpaces(std::istream& input) {
    while (input.peek() != std::char_traits<char>::eof() && std::isspace(input.peek())) input.get();
}

inline Integer ReadInteger(std::istream& input) {
    std::string digits;
    if (input.peek() == '-' || input.peek() == '+') digits += static_cast<char>(input.get());
    const std::size_t sign = digits.size();
    while (input.peek() >= '0' && input.peek() <= '9') digits += static_cast<char>(input.get());
    if (digits.size() == sign) throw interpret_error("integer, string or array expected on input");
    const char* first = digits.data() + (digits[0] == '+' ? 1 : 0);
    Integer value;
    auto result = std::from_chars(first, digits.data() + digits.size(), value);
    if (result.ec != std::errc()) throw interpret_error("integer on input is out of range");
    return value;
}

inline StringRef ReadString(std::istream& input) {
    input.get();
    auto text = std::make_shared<std::string>();
    while (true) {
        const int ch = input.get();
        if (ch == std::char_traits<char>::eof()) throw interpret_error("unterminated string on input");
        if (ch == '"') return text;
        *text += static_cast<char>(ch);
    }
}

inline Value ReadValue(std::istream& input) {
    SkipSpaces(input);
    while (input.peek() == ',') {
        input.get();
        SkipSpaces(input);
    }
    if (input.peek() == std::char_traits<char>::eof()) throw interpret_error("input exhausted");
    std::vector<ArrayRef> open;
    while (true) {
        SkipSpaces(input);
        if (input.peek() == '[') {
            input.get();
            open.push_back(std::make_shared<ArrayData>());
            SkipSpaces(input);
            if (input.peek() == ']') throw interpret_error("empty array on input");
            continue;
        }
        Value current = input.peek() == '"' ? Value(ReadString(input)) : Value(ReadInteger(input));
        while (true) {
            if (open.empty()) return current;
            open.back()->elements.push_back(std::move(current));
            SkipSpaces(input);
            const int next = input.get();
            if (next == ',') break;
            if (next != ']') throw interpret_error("',' or ']' expected on input");
            current = std::move(open.back());
            open.pop_back();
        }
    }
}

}
}
