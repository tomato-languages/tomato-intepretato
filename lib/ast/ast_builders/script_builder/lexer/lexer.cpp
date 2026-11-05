#include "ast/ast_builders/script_builder/lexer/lexer.h"
#include <cctype>

namespace NTomatoInterpretato::NAst {

int Lexer::take() {
    int ch = input_.get();
    if (ch == '\n') {
        ++position_.line;
        position_.column = 1;
    } else if (ch != std::char_traits<char>::eof()) {
        ++position_.column;
    }
    return ch;
}

Token Lexer::next() {
    while (true) {
        while (input_.peek() != std::char_traits<char>::eof() &&
               std::isspace(static_cast<unsigned char>(input_.peek()))) take();
        Pos start = position_;
        int ch = take();
        if (ch == std::char_traits<char>::eof()) return {Token::Kind::End, "", start};
        if (ch == '-' && input_.peek() == '-') {
            while (input_.peek() != '\n' && input_.peek() != std::char_traits<char>::eof()) take();
            continue;
        }
        if (ch == '(' && input_.peek() == '*') {
            take();
            while (true) {
                int current = take();
                if (current == std::char_traits<char>::eof()) throw ast_error("Unterminated comment", start);
                if (current == '*' && input_.peek() == ')') {
                    take();
                    break;
                }
            }
            continue;
        }
        std::string text(1, static_cast<char>(ch));
        if (ch >= 'a' && ch <= 'z') {
            while (input_.peek() != std::char_traits<char>::eof()) {
                int next = input_.peek();
                if (!std::isalnum(static_cast<unsigned char>(next)) && next != '_' && next != '\'') break;
                text += static_cast<char>(take());
            }
            return {Token::Kind::Identifier, text, start};
        }
        if (ch >= '0' && ch <= '9') {
            while (input_.peek() >= '0' && input_.peek() <= '9') text += static_cast<char>(take());
            return {Token::Kind::Number, text, start};
        }
        if ((ch == '!' && (input_.peek() == '!' || input_.peek() == '=')) ||
            (ch == '&' && input_.peek() == '&') ||
            ((ch == '=' || ch == '<' || ch == '>') && input_.peek() == '=')) {
            text += static_cast<char>(take());
            return {Token::Kind::Symbol, text, start};
        }
        if (text.find_first_not_of("{}();=+-*/%<>") == std::string::npos) {
            return {Token::Kind::Symbol, text, start};
        }
        throw ast_error("Unexpected character: " + text, start);
    }
}

}
