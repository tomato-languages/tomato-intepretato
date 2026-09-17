#pragma once
#include <cstring>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <exception>
#include "errors.h"

namespace ItmoScript {
    


enum class TokenType {
    If, Then, Else, ElseIf,
    While,
    For, In,
    Break, Continue, End, 

    // Функции
    Function, Return,

    // Логические литералы
    True, False, Nil,

    // Стандартные операторы
    And, Or, Not,

    // Общие
    Identifier,

    // Арифметические
    Plus,        // +
    Minus,       // -
    Star,        // *
    Slash,       // /
    Percent,     // %
    Caret,       // ^

    // Присваивания
    Assign,          // =
    PlusAssign,      // +=
    MinusAssign,     // -=
    StarAssign,      // *=
    SlashAssign,     // /=
    PercentAssign,   // %=
    CaretAssign,     // ^=

    // Сравнения
    Equal,           // ==
    NotEqual,        // !=
    Less,            // <
    Greater,         // >
    LessEqual,       // <=
    GreaterEqual,    // >=

    Comma,        // ,
    Colon,        // :
    LParen,       // (
    RParen,       // )
    LBracket,     // [
    RBracket,     // ]

    // Литералы
    Number, 
    String,

    // Специальные
    Comment,
    EndOfFile,
    Unknown

};

struct Token {
    TokenType type = TokenType::Unknown;
    std::string value;
    Pos position;

    Token& operator=(const Token& other) {
        if (this != &other) {
            type = other.type;
            value = other.value;
            position = other.position;
        }
        return *this;
    }
};

class Lexer {
public:

    Lexer(std::istream& input, Pos current_position = {1,1}) 
        : stream_(input)
        , current_position_(current_position)
    {}

    Token PeekToken();
    Token NextToken();
    
    Pos current_position_;
private:
    
    std::istream& stream_;


    char StreamGet() {
        char c = stream_.get();
        current_position_.column++;

        if (c == '\n') {
            current_position_.line++;
            current_position_.column = 1;
        }

        return c;
    }
    
    void StreamUnget() {
        stream_.unget();
        current_position_.column--;

        if (current_position_.column < 1) {
            current_position_.line--;
            current_position_.column = 1;
        }
    }
    
    Token SkipComment();
    Token IdentifierOrKeyword();
    Token Number();
    Token String();
    Token Symbol();
    
    static std::unordered_map<std::string, TokenType> keywords;
    Token current_token_;

};





} // namespace ItmoScript