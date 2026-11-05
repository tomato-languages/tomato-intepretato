#pragma once
#include <stdexcept>
#include <string>

namespace NTomatoInterpretato {

struct Pos {
    int line = 0;
    int column = 0;

    Pos(int line = 0, int column = 0) : line(line), column(column) {}

    std::string to_string() const {
        return "Line: " + std::to_string(line) + ", Column: " + std::to_string(column) + ". ";
    }

    explicit operator bool() const {
        return line != 0 || column != 0;
    }
};

class basic_error : public std::runtime_error {
public:
    explicit basic_error(const std::string& message, Pos position = Pos())
        : std::runtime_error((position ? position.to_string() : "") + message) {}
};

class interpret_error : public basic_error {
public:
    using basic_error::basic_error;
};

class ast_error : public basic_error {
public:
    using basic_error::basic_error;
};

}
