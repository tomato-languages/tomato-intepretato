#pragma once
#include <istream>
#include <string>
#include "common/errors.h"

namespace NTomatoInterpretato::NAst {

struct Token {
    enum class Kind { End, Identifier, Number, Symbol, String };
    Kind kind;
    std::string text;
    Pos position;
};

class Lexer {
public:
    explicit Lexer(std::istream& input) : input_(input) {}
    Token next();

private:
    std::istream& input_;
    Pos position_{1, 1};
    int take();
};

}
