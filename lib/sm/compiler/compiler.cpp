#include "sm/compiler/compiler.h"

namespace NTomatoInterpretato::NSm {

Program Compiler::compile(const NAst::AST& ast) const {
    Program program;
    for (const auto& stmt : ast) stmt->compile(program);
    program.emit_end();
    return program;
}

}
