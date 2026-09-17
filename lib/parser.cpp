#include "parser.h"

namespace ItmoScript {
using ExprNode = std::unique_ptr<BaseExpr>;




ExprNode Parser::ParseExpression() {
    return ParseAssignment();

}

ExprNode Parser::ParseAssignment() {
    ExprNode lhs = ParseLogicOr();

    Token token = lexer_.PeekToken();

    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::Assign, TokenType::PlusAssign, 
                     TokenType::MinusAssign, TokenType::StarAssign,
                     TokenType::SlashAssign, TokenType::PercentAssign, TokenType::CaretAssign)) {

        Token op = token;
        lexer_.NextToken();
        ExprNode rhs = ParseLogicOr();

        if (op.type == TokenType::Assign) {
            return ExprNode(new AssignExpr(std::move(lhs), std::move(rhs), token.position));
        } else {

            TokenType bin_op;
            switch (op.type) {
                case TokenType::PlusAssign:    bin_op = TokenType::Plus; break;
                case TokenType::MinusAssign:   bin_op = TokenType::Minus; break;
                case TokenType::StarAssign:    bin_op = TokenType::Star; break;
                case TokenType::SlashAssign:   bin_op = TokenType::Slash; break;
                case TokenType::PercentAssign: bin_op = TokenType::Percent; break;
                case TokenType::CaretAssign:   bin_op = TokenType::Caret; break;
            }

            VariableExpr* varExpr = dynamic_cast<VariableExpr*>(lhs.get());
            if (!varExpr) {
                throw parsing_error("Left-hand side of assignment must be a variable", op.position);
            }
            ExprNode lhs_clone = ExprNode(new VariableExpr(varExpr->var_name, varExpr->position));

            ExprNode combined = ExprNode(new BinaryOpExpr(bin_op, std::move(lhs_clone), std::move(rhs), token.position));
            return ExprNode(new AssignExpr(std::move(lhs), std::move(combined), token.position));
        }
    }


    return lhs;
}

ExprNode Parser::ParseLogicOr() {
    ExprNode lhs = ParseLogicAnd();

      Token token = lexer_.PeekToken();

    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::Or)) {
        TokenType op = token.type;
        lexer_.NextToken();

        ExprNode rhs = ParseLogicAnd();
        lhs = ExprNode(new BinaryOpExpr(op, std::move(lhs), std::move(rhs), token.position));

    }  

    return lhs;

}

ExprNode Parser::ParseLogicAnd() {
    ExprNode lhs = ParseEquality();

    Token token = lexer_.PeekToken();

    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::And)) {
        TokenType op = token.type;
        lexer_.NextToken();

        ExprNode rhs = ParseEquality();
        lhs = ExprNode(new BinaryOpExpr(op, std::move(lhs), std::move(rhs), token.position));

    }

    return lhs;

}
ExprNode Parser::ParseEquality(){
    ExprNode lhs = ParseComparison();

    Token token = lexer_.PeekToken();
    
    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::Equal, TokenType::NotEqual)) {
        TokenType op = token.type;
        lexer_.NextToken();

        ExprNode rhs = ParseComparison();
        lhs = ExprNode(new BinaryOpExpr(op, std::move(lhs), std::move(rhs), token.position));

    }


    return lhs;
    
}

ExprNode Parser::ParseComparison(){
    ExprNode lhs = ParseAdditive();

    Token token = lexer_.PeekToken();

    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::Less, TokenType::Greater, TokenType::LessEqual, TokenType::GreaterEqual)) {
        TokenType op = token.type;
        lexer_.NextToken();

        ExprNode rhs = ParseAdditive();
        lhs = ExprNode(new BinaryOpExpr(op, std::move(lhs), std::move(rhs), token.position));

    }

    return lhs;
}

ExprNode Parser::ParseAdditive(){
    ExprNode lhs = ParseMultiplicative();

    Token token = lexer_.PeekToken();

    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::Plus, TokenType::Minus)) {
        TokenType op = token.type;
        lexer_.NextToken();

        ExprNode rhs = ParseMultiplicative();
        lhs = ExprNode(new BinaryOpExpr(op, std::move(lhs), std::move(rhs), token.position));

    }


    return lhs;

}

ExprNode Parser::ParseMultiplicative() {
    ExprNode lhs = ParseUnary();

    Token token = lexer_.PeekToken();

    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::Star, TokenType::Slash, TokenType::Percent)) {
        TokenType op = token.type;
        lexer_.NextToken();

        ExprNode rhs = ParseUnary();
        lhs = std::make_unique<BinaryOpExpr>(op, std::move(lhs), std::move(rhs), token.position);

    }


    return lhs;

}

ExprNode Parser::ParseUnary() {
    Token token = lexer_.PeekToken();

    while (EqualAtLeast(lexer_.PeekToken().type, TokenType::Minus, TokenType::Plus, TokenType::Not)) {
        lexer_.NextToken(); 

        ExprNode operand = ParseUnary();  

        // Оборачиваем в унарное выражение
        return std::make_unique<UnaryOpExpr>(token.type, std::move(operand), token.position);
    }

    return ParsePrimary();
}

ExprNode Parser::ParsePrimary() {
    const Token& token = lexer_.PeekToken();

    switch (token.type) {

        case TokenType::Number: {
            lexer_.NextToken();
            return ExprNode(new NumberExpr(std::strtod(token.value.c_str(), nullptr), token.position));
        }
        case TokenType::String: {
            lexer_.NextToken();

            return ExprNode(new StringExpr(token.value, token.position));
        }

        case TokenType::True:
        case TokenType::False: {
            lexer_.NextToken();
            return ExprNode(new NumberExpr(token.type == TokenType::True ? 1.0 : 0.0, token.position));
        }

        case TokenType::Nil: {
            lexer_.NextToken();
            return std::make_unique<NilExpr>(token.position); 
        }

        case TokenType::Function: {
            lexer_.NextToken();

            if (lexer_.PeekToken().type != TokenType::LParen) {
                throw parsing_error("Left paren expected after function keyword", lexer_.PeekToken().position);
            }   

            std::vector<ExprNode> args;
            std::vector<StmtNode> body;


            lexer_.NextToken(); // Skip LParen

            while (lexer_.PeekToken().type != TokenType::RParen) {

                if (lexer_.PeekToken().type != TokenType::Identifier) {
                    throw parsing_error("Identifier expected for function argument", lexer_.PeekToken().position);
                } 

                args.push_back(ParsePrimary());

                if (lexer_.PeekToken().type == TokenType::Comma) {
                    lexer_.NextToken(); // Skip comma
                }

                
            }

            lexer_.NextToken(); // Skip RParen

            while (lexer_.PeekToken().type != TokenType::End) {
                body.push_back(StmtNode(ParseStatement()));
            }

            lexer_.NextToken(); 
            if (lexer_.PeekToken().type != TokenType::Function) {
                throw parsing_error("Expected function keyword after end", lexer_.PeekToken().position);
            }
            lexer_.NextToken(); 


            return ExprNode(new FunctionExpr(std::move(args), std::move(body), token.position));
        }

        case TokenType::Identifier: {
            
            ExprNode var_node = ExprNode(new VariableExpr(lexer_.PeekToken().value, lexer_.PeekToken().position));

            lexer_.NextToken();

            return ParsePostfix(std::move(var_node));
  

        }

        case TokenType::LParen: {
            lexer_.NextToken();

            auto expr = ParseExpression(); 
            
            if (lexer_.PeekToken().type != TokenType::RParen) {
                throw parsing_error("Right paren expected", lexer_.PeekToken().position);
            }

            lexer_.NextToken();

            return expr;
        }

        case TokenType::LBracket: {
            lexer_.NextToken();

            std::vector<ExprNode> list_elements;
            std::vector<KV> dict_elements;
            
            if (lexer_.PeekToken().type == TokenType::RBracket) {
                lexer_.NextToken();
                return ExprNode(new ListExpr(std::move(list_elements), lexer_.PeekToken().position));
            }
            bool is_dict = false;

            ExprNode key_or_elem = ParseExpression();

                if (lexer_.PeekToken().type == TokenType::Colon){
                    is_dict = true;

                    lexer_.NextToken();


                    ExprNode value = ParseExpression();
                    dict_elements.push_back({std::move(key_or_elem), std::move(value)});
                } else {
                    list_elements.push_back(std::move(key_or_elem));

                }

                if (lexer_.PeekToken().type == TokenType::Comma) {
                    lexer_.NextToken(); // Skip comma
                }


                
            while (lexer_.PeekToken().type != TokenType::RBracket) {
                if (is_dict) {
                    ExprNode key = ParseExpression();

                    if (lexer_.PeekToken().type != TokenType::Colon) {
                        throw parsing_error("Expected colon in dict", lexer_.PeekToken().position);
                    }

                    ExprNode value = ParseExpression();

                    dict_elements.push_back({std::move(key), std::move(value)});

                    if (lexer_.PeekToken().type == TokenType::Comma) {
                        lexer_.NextToken(); // Skip comma
                    }   

                } else {

                    ExprNode value = ParseExpression();

                    list_elements.push_back(std::move(value));

                    if (lexer_.PeekToken().type == TokenType::Comma) {
                        lexer_.NextToken(); // Skip comma
                    } 

                }

                
            }
            
            lexer_.NextToken();
            if (is_dict) {
                return ExprNode(new DictionaryExpr(std::move(dict_elements), lexer_.PeekToken().position));
            } else {
                return ExprNode(new ListExpr(std::move(list_elements), lexer_.PeekToken().position));
            }           
            
        }

        default:
            throw parsing_error("Expression expected", lexer_.PeekToken().position);
    }

}

ExprNode Parser::ParsePostfix(ExprNode&& var_node) {
    ExprNode lhs = std::move(var_node);

    switch (lexer_.PeekToken().type) {
    case TokenType::LParen: {

        lexer_.NextToken(); // Skip LParen

        std::vector<ExprNode> args;

        while (lexer_.PeekToken().type != TokenType::RParen) {
            args.push_back(ParseExpression());
            
            if (lexer_.PeekToken().type == TokenType::Comma) {
                lexer_.NextToken(); // Skip comma
            }
        }

        lexer_.NextToken(); // Skip RParen

        return ParsePostfix(ExprNode(new CallableExpr(std::move(lhs), std::move(args), lexer_.PeekToken().position)));
    }

    case TokenType::LBracket: {

        lexer_.NextToken(); // Skip LBracket

        std::optional<ExprNode> start;
        std::optional<ExprNode> end;
        std::optional<ExprNode> step;

        if (lexer_.PeekToken().type != TokenType::Colon) {
            
            start = ParseExpression();

            switch (lexer_.PeekToken().type) {
            case TokenType::RBracket: {
                lexer_.NextToken(); 
                return ParsePostfix(ExprNode(new IndexExpr(std::move(lhs), std::move(start.value()))));
            }
            case TokenType::Colon: {
                lexer_.NextToken();

                if (lexer_.PeekToken().type == TokenType::RBracket) {
                    lexer_.NextToken(); 
                    return ParsePostfix(ExprNode(new SliceExpr(std::move(lhs), std::move(start), std::move(end), std::move(step))));
                }

                if (lexer_.PeekToken().type != TokenType::RBracket && lexer_.PeekToken().type != TokenType::Colon) {
                    end = ParseExpression();
                }
                
                if (lexer_.PeekToken().type == TokenType::Colon) {
                    lexer_.NextToken();

                    if (lexer_.PeekToken().type != TokenType::RBracket) {
                        step = ParseExpression();
                    }
                }

                if (lexer_.PeekToken().type != TokenType::RBracket) {
                    throw parsing_error("Right bracket ] expected", lexer_.PeekToken().position);
                }
                lexer_.NextToken();

                return ParsePostfix(ExprNode(new SliceExpr(std::move(lhs), std::move(start), std::move(end), std::move(step))));
            }
            
            default:
                throw parsing_error("Right bracket ] expected", lexer_.PeekToken().position);

            }

        } else {

            lexer_.NextToken();

            if (lexer_.PeekToken().type != TokenType::RBracket && lexer_.PeekToken().type != TokenType::Colon) {
                end = ParseExpression();
            }

           if (lexer_.PeekToken().type == TokenType::Colon) {
                lexer_.NextToken();

                if (lexer_.PeekToken().type != TokenType::RBracket) {
                    step = ParseExpression();
                }
            }

            if (lexer_.PeekToken().type != TokenType::RBracket) {
                throw parsing_error("Right bracket ] expected", lexer_.PeekToken().position);
            }

            lexer_.NextToken();

            return ParsePostfix(ExprNode(new SliceExpr(std::move(lhs), std::move(start), std::move(end), std::move(step))));

        }
        
    }
    
    default:
        return lhs;
    }


}

StmtNode Parser::ParseStatement() {
    switch (lexer_.PeekToken().type) {
    case TokenType::Identifier: {
    
        return StmtNode(new ExprStmt(ParseExpression()));
    }

    case TokenType::If: {

        return ParseIfStmt();

    }

    case TokenType::While: {

        return ParseWhileStmt();

    }

    case TokenType::For: {

        return ParseForStmt();

    }

    case TokenType::Return: {
        Token token = lexer_.NextToken();

        ExprNode return_value = ParseExpression();

        return StmtNode(new ReturnStmt(std::move(return_value)));
    }

    case TokenType::Break: {
        Token token = lexer_.NextToken();

        return StmtNode(new BreakStmt());
    }

    case TokenType::Continue: {
        Token token = lexer_.NextToken();

        return StmtNode(new ContinueStmt());
    }

    default:
        throw parsing_error("Statement expected", lexer_.PeekToken().position);
    }

}


StmtNode Parser::ParseIfStmt() {
    Token token = lexer_.NextToken();

    ExprNode condition = ParseExpression();
    std::vector<StmtNode> body;
    std::vector<StmtNode> else_body;                

    token = lexer_.PeekToken();


    if (token.type == TokenType::Then) {
        token = lexer_.NextToken();
        while (!EqualAtLeast(lexer_.PeekToken().type, TokenType::End, TokenType::ElseIf, TokenType::Else)) {
            body.push_back(StmtNode(ParseStatement()));
        }


        IfLabel:

        token = lexer_.PeekToken();


        switch(token.type) {

            case (TokenType::Else): {
                token = lexer_.NextToken();

                if (token.type == TokenType::If) {
                    token = lexer_.NextToken();

                    ExprNode condition1 = ParseExpression();
                    std::vector<StmtNode> body1;

                    token = lexer_.PeekToken();


                    if (token.type == TokenType::Then) {
                        token = lexer_.NextToken();
                        while (!EqualAtLeast(lexer_.PeekToken().type, TokenType::End, TokenType::Else)) {
                            body1.push_back(StmtNode(ParseStatement()));
                        }
                        token = lexer_.PeekToken();

                        StmtNode else_if_node = StmtNode(new IfStmt(std::move(condition1), std::move(body1)));
                        else_body.push_back(std::move(else_if_node));

                        goto IfLabel;

                    } else {
                        throw parsing_error("Excepted then after else if condition", lexer_.PeekToken().position);
                    }
                } else {
                    ExprNode condition1 = ExprNode(new NumberExpr(1));
                    std::vector<StmtNode> body1;

                    token = lexer_.PeekToken();

                    while (!EqualAtLeast(lexer_.PeekToken().type, TokenType::End)) {
                        body1.push_back(StmtNode(ParseStatement()));
                    }

                    StmtNode else_if_node = StmtNode(new IfStmt(std::move(condition1), std::move(body1)));
                    else_body.push_back(std::move(else_if_node));

                    goto IfLabel;

                }


            }

            case (TokenType::End): {
                token = lexer_.NextToken();
                if (token.type == TokenType::If) {
                    token = lexer_.NextToken();
                    break;
                } else {
                    throw parsing_error("Excepted if keyword after keyword end", lexer_.PeekToken().position);
                }


            }

            default:
                throw parsing_error("Expected end keyword after if keyword", lexer_.PeekToken().position);

        }



    } else {
        throw parsing_error("Excepted then after if condition", lexer_.PeekToken().position);
    }



    return StmtNode(new IfStmt(std::move(condition), std::move(body), std::move(else_body)));

}


StmtNode Parser::ParseWhileStmt() {
    Token token = lexer_.NextToken();

    ExprNode condition = ParseExpression();
    std::vector<StmtNode> body;
    std::vector<StmtNode> else_body;                

    while (!EqualAtLeast(lexer_.PeekToken().type, TokenType::Else, TokenType::End)) {
        body.push_back(ParseStatement());
    }

    switch (lexer_.PeekToken().type) {
    case TokenType::Else: {

        while (!EqualAtLeast(lexer_.PeekToken().type, TokenType::End)) {
            else_body.push_back(ParseStatement());
        }

        goto EndWhile;
    }   

    case TokenType::End: {
        EndWhile:
        lexer_.NextToken();
        
        if (lexer_.PeekToken().type == TokenType::While) {
            lexer_.NextToken();
            break;
        } else {
            throw parsing_error("Expeceted keyword WHILE after keywork END", lexer_.PeekToken().position); 
        }

    }   
    
    default:
        break;
    }


    return StmtNode(new WhileStmt(std::move(condition) ,std::move(body), std::move(else_body)));

}

StmtNode Parser::ParseForStmt() {

    lexer_.NextToken();

    if (lexer_.PeekToken().type != TokenType::Identifier) {
        throw parsing_error("Identifier expected for FOR index variable", lexer_.PeekToken().position);
    }

    std::string index_var_name = lexer_.PeekToken().value;

    lexer_.NextToken();

    if (lexer_.PeekToken().type != TokenType::In) {
        throw parsing_error("Expected IN keyword after FOR index variable", lexer_.PeekToken().position);
    }

    lexer_.NextToken(); 
    
    ExprNode collection_expr = ParseExpression();
    
    std::vector<StmtNode> body;

    while (lexer_.PeekToken().type != TokenType::End) {
        body.push_back(ParseStatement());
    }

    lexer_.NextToken(); 

    if (lexer_.PeekToken().type != TokenType::For) {
        throw parsing_error("Expected END keyword after FOR body", lexer_.PeekToken().position);
    }

    lexer_.NextToken(); 

    return StmtNode(new ForStmt(std::move(collection_expr), std::move(body), index_var_name));

}

};