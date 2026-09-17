#pragma once
#include <fstream>
#include <vector>
#include <unordered_map>
#include "common/errors.h"
#include "ast/ast.h"

namespace NTomatoInterpretato {
namespace NAst {

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

}
}
