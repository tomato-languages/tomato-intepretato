#pragma once
#include <exception>
#include <string>

namespace TomatoInterpretato {

struct Pos {
    int line;
    int column;

    std::string to_string() const {
        return "Line: " + std::to_string(line) + ", Column: " + std::to_string(column) + ". ";
    }

    operator bool() const {
        if (line == 0 && column == 0) return false;
        return true;
    }

    Pos(int l = 0, int c = 0) : line(l), column(c) {}
};

class basic_error : public std::exception {
public:
    basic_error(std::string&& msg, Pos position)
        : msg_(std::move(msg))
        , pos_(position)
    {
        update_what();
    }

    const char* what() const noexcept override {
        return msg_.c_str();
    }

protected:
    std::string msg_;
    Pos pos_;

    void update_what() {
        if (pos_) {
            msg_ = pos_.to_string() + msg_;
        }
    }

};

// Malformed JSON text
class json_error : public basic_error {
public:
    json_error(std::string&& msg, Pos position)
        : basic_error(std::move(msg), position)
    {}
};

// Well-formed JSON that is not a valid AST
class ast_error : public basic_error {
public:
    ast_error(std::string&& msg, Pos position = Pos())
        : basic_error(std::move(msg), position)
    {}
};

// Runtime error during program execution
class interpret_error : public basic_error {
public:
    interpret_error(std::string&& msg, Pos position = Pos())
        : basic_error(std::move(msg), position)
    {}
};

};
