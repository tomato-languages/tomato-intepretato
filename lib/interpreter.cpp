#include "interpreter.h"

namespace TomatoInterpretato {

void Interpreter::interpret(std::istream& ast_stream) {
    if (!ast_stream) {
        throw interpret_error("Input stream is not valid");
    }

    // operator>> stops right after the JSON value, the rest stays for `read`
    nlohmann::json json;
    ast_stream >> json;

    StmtNode program = builder_.build_program(json);

    program->execute(env_);
    env_.output.flush();
}

};
