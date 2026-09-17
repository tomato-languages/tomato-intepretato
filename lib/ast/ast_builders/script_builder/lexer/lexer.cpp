#include "lexer.h"
#include <iostream>
#include <cctype>

namespace NTomatoInterpretato {
namespace NAst {

std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"if", TokenType::If},
    {"then", TokenType::Then},
    {"else", TokenType::Else},

    {"while", TokenType::While},
    {"for", TokenType::For},
    {"in", TokenType::In},
    {"end", TokenType::End},

    {"break", TokenType::Break},
    {"continue", TokenType::Continue},

    {"function", TokenType::Function},
    {"return", TokenType::Return},

    {"true", TokenType::True},
    {"false", TokenType::False},
    {"nil", TokenType::Nil},

    {"and", TokenType::And},
    {"or", TokenType::Or},
    {"not", TokenType::Not},

    {"=", TokenType::Assign},

    {"==", TokenType::Equal},
    {"!=", TokenType::NotEqual},

    {">", TokenType::Greater},
    {">=", TokenType::GreaterEqual},
    
    {"<", TokenType::Less},
    {"<=", TokenType::LessEqual},

    {"+", TokenType::Plus},
    {"+=", TokenType::PlusAssign},
    {"-", TokenType::Minus},
    {"-=", TokenType::MinusAssign},
    {"*", TokenType::Star},
    {"*=", TokenType::StarAssign},
    {"/", TokenType::Slash},
    {"/=", TokenType::SlashAssign},
    {"^", TokenType::Caret},
    {"^=", TokenType::CaretAssign},
    {"%", TokenType::Percent},
    {"%=", TokenType::PercentAssign},

    {",", TokenType::Comma},
    {":", TokenType::Colon},
    {"(", TokenType::LParen},
    {")", TokenType::RParen},
    {"[", TokenType::LBracket},
    {"]", TokenType::RBracket}

};


Token Lexer::NextToken() {

    while (!stream_.eof() && (stream_.peek() == ' ' || stream_.peek() == '\n' || stream_.peek() == '\t')) StreamGet();

    
    if (stream_.eof()) {
        return current_token_ = Token{TokenType::EndOfFile, "", current_position_};
    }
    
    char peek = stream_.peek();

    if (peek == '/') {
        if (StreamGet() == '/') {

        return current_token_ = SkipComment();
        } else {
            StreamUnget();
        }
    }


    if (isalpha(peek) || peek == '_') {
        return current_token_ = IdentifierOrKeyword();
    } else if (isdigit(peek)) {
        return current_token_ = Number();
    } else if (peek == '"') {
        return current_token_ = String();
    } else { 
        return current_token_ = Symbol();
    }
}

Token Lexer::PeekToken() {


    return current_token_;
}



Token Lexer::SkipComment() {

    StreamGet();
    while (stream_.peek() != '\n' && !stream_.eof()) StreamGet();
    StreamGet();

    return Token{TokenType::Comment, "", current_position_};

}

Token Lexer::IdentifierOrKeyword() {

    std::string value;

    while (isalnum(stream_.peek()) || stream_.peek() == '_') value += StreamGet();

    if (keywords.contains(value)) {
        return Token{keywords[value], value};
    } else {
        return Token{TokenType::Identifier, value, current_position_};
    }

}

Token Lexer::Number() {

    std::string value;

    while (std::isdigit(stream_.peek())) {
        value += StreamGet();
    }

    if (stream_.peek() == '.') {
        value += StreamGet();
        while (std::isdigit(stream_.peek())) {
            value += StreamGet();
        }
    }

    if (stream_.peek() == 'e' || stream_.peek() == 'E') {
        value += StreamGet();
        if (stream_.peek() == '+' || stream_.peek() == '-') {
            value += StreamGet();
        }
        while (std::isdigit(stream_.peek())) {
            value += StreamGet();
        }
    }


    return Token{TokenType::Number, value, current_position_};

}

Token Lexer::String() {

    std::string value;

    StreamGet();

    while (stream_.peek() != '"' && !stream_.eof()) {
        if (stream_.peek() == '\\') {
            StreamGet();
            value += StreamGet();
        } else {
            value += StreamGet();

        }
    }

    StreamGet();

    return Token{TokenType::String, value, current_position_};


}

Token Lexer::Symbol() {

    char c1 = StreamGet();
    char c2 = StreamGet();
    std::string symbol(1, c1);
    if (!stream_.eof()) {
        if (c2 == '=') {
            symbol += c2;
        } else if (c2 != 0) {
            StreamUnget();
        }
    }

    if (keywords.contains(symbol)) {
        return Token{keywords[symbol], symbol, current_position_};
    } else {
        throw lexer_error("Unknown token", current_position_);
    }

}


}

}
