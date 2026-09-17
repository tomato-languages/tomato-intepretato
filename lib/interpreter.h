#pragma once
#include <iostream>
#include "lexer.h"
#include "parser.h"
#include "functions.h"
bool interpret(std::istream& input, std::ostream& output);

namespace ItmoScript {
    
class Interpreter {
public:

    Interpreter(std::ostream& output = std::cout) 
        : output_(output)
        , ast_()
        , env_()
        , lex_position_(1,1)
    {

        env_["print"] = Value(std::make_shared<PrintFunction>(output_));
        env_["println"] = Value(std::make_shared<PrintlnFunction>(output_));
        env_["range"] = Value(std::make_shared<RangeFunction>());
        env_["abs"] = Value(std::make_shared<AbsFunction>());
        env_["ceil"] = Value(std::make_shared<CeilFunction>());
        env_["floor"] = Value(std::make_shared<FloorFunction>());
        env_["round"] = Value(std::make_shared<RoundFunction>());
        env_["sqrt"] = Value(std::make_shared<SqrtFunction>());
        env_["rnd"] = Value(std::make_shared<RndFunction>());
        env_["parse_num"] = Value(std::make_shared<ParseNumFunction>());
        env_["to_string"] = Value(std::make_shared<ToStringFunction>());
        env_["len"] = Value(std::make_shared<LenFunction>());
        env_["lower"] = Value(std::make_shared<LowerFunction>());
        env_["upper"] = Value(std::make_shared<UpperFunction>());
        env_["split"] = Value(std::make_shared<SplitFunction>());
        env_["join"] = Value(std::make_shared<JoinFunction>());
        env_["replace"] = Value(std::make_shared<ReplaceFunction>());
        env_["push"] = Value(std::make_shared<PushFunction>());
        env_["pop"] = Value(std::make_shared<PopFunction>());
        env_["insert"] = Value(std::make_shared<InsertFunction>());
        env_["remove"] = Value(std::make_shared<RemoveFunction>());
        env_["sort"] = Value(std::make_shared<SortFunction>());
        env_["read"] = Value(std::make_shared<ReadFunction>());

    }

    bool interpret(std::istream& istream) {
        
        if (!istream) {
            throw interpret_error("Input stream is not valid");
        }

        Lexer lexer(istream, lex_position_);
        Parser parser(lexer, ast_);
        if (!parser.parse()) return false;

        for (auto& it : ast_) {
            it->execute(env_);
            ast_.pop_front();   
        }
        
        lexer.current_position_.line++;
        lexer.current_position_.column = 1;

        lex_position_ = lexer.current_position_;
    
        
        return true;

    }


private:

    std::ostream& output_;

    Pos lex_position_;

    AST ast_;
    Environment env_;

};

};