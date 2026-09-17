#include "interpreter.h"

bool interpret(std::istream& ast_input, std::istream& input, std::ostream& output) {
    NTomatoInterpretato::Interpreter interpretator(input, output);
    
    return interpretator.interpret(ast_input);
    
}
