#include "interpreter.h"

namespace TomatoInterpretato {

void Interpreter::interpret(std::istream& ast_stream) {
    if (!ast_stream) {
        throw interpret_error("Input stream is not valid");
    }

    JsonReader reader(ast_stream);
    Json json = reader.read();

    StmtNode program = builder_.build_program(json);

    program->execute(env_);
    env_.output.flush();
}

};
