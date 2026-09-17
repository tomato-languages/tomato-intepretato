#include "lexer.h"
#include "ast.h"

template <typename T, typename... Args>
bool EqualAtLeast(T lhs, Args... rhs) {
    if (((lhs == rhs) || ...)) {
        return true;
    }
    return false;
}

namespace ItmoScript {

    
class Parser {

public:

    Parser(Lexer& lexer, AST& ast)
        : lexer_(lexer)
        , ast_(ast)
    {}

    bool parse() {
        
        Token token = lexer_.NextToken();
        
        if (token.type == TokenType::EndOfFile) {
            return false;
        }

        while (lexer_.PeekToken().type != TokenType::EndOfFile) {
            
            if (lexer_.PeekToken().type == TokenType::Comment) {
                token = lexer_.NextToken();
                continue;
            }

            ast_.push_back(ParseStatement());
        }
        
 
        return true;

    }




private:
    Lexer& lexer_;
    AST& ast_;



    ExprNode ParseExpression();
    ExprNode ParseAssignment();
    ExprNode ParseLogicOr();
    ExprNode ParseLogicAnd();
    ExprNode ParseEquality();
    ExprNode ParseComparison();
    ExprNode ParseAdditive();
    ExprNode ParseMultiplicative();
    ExprNode ParseUnary();
    ExprNode ParsePrimary();
    ExprNode ParsePostfix(ExprNode&& var_node);

    StmtNode ParseStatement();
    StmtNode ParseIfStmt();
    StmtNode ParseWhileStmt();
    StmtNode ParseForStmt();
};




 
}; // namespace