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

    virtual const char* what() const noexcept override = 0;

protected:
    std::string msg_;
    Pos pos_;
    // debug info in future

    void update_what() {
        if (pos_) {
            msg_ = pos_.to_string() + msg_;
        }
    }

   

};

class parsing_error : public basic_error {
public:

    parsing_error(std::string&& msg, Pos position) 
        : basic_error(std::move(msg), position)
    {}

    const char* what() const noexcept override {
        return msg_.c_str();
    }


};

class lexer_error : public basic_error {
public:

    lexer_error(std::string&& msg, Pos position) 
        : basic_error(std::move(msg), position)
    {}

    const char* what() const noexcept override {
        return msg_.c_str();
    }


};

class interpret_error : public basic_error {
public:

    interpret_error(std::string&& msg, Pos position = Pos()) 
        : basic_error(std::move(msg), position)
    {}

    const char* what() const noexcept override {
        return msg_.c_str();
    }

};

// JSON that does not describe a valid AST
class ast_error : public basic_error {
public:

    ast_error(std::string&& msg, Pos position = Pos())
        : basic_error(std::move(msg), position)
    {}

    const char* what() const noexcept override {
        return msg_.c_str();
    }

};

};
