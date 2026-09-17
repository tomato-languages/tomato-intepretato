#include "interpreter.h"

bool interpret(std::istream& input, std::ostream& output) {
    ItmoScript::Interpreter interpretator(output);
    
    return interpretator.interpret(input);
    
}
namespace ItmoScript {

    
};