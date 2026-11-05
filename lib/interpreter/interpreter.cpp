#include "interpreter/interpreter.h"

namespace NTomatoInterpretato {

void Interpreter::interpret(const NAst::AST& ast) {
    env_.clear();
    for (const auto& stmt : ast) stmt->execute(env_);
}

}
