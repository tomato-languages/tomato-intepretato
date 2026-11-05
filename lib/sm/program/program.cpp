#include "sm/program/program.h"
#include <sstream>

namespace NTomatoInterpretato {
namespace NSm {

std::string to_string(const Instruction& instruction) {
    switch (instruction.opcode) {
        case EOpcode::Const:       return "CONST " + std::to_string(instruction.value);
        case EOpcode::Ld:          return "LD " + instruction.name;
        case EOpcode::St:          return "ST " + instruction.name;
        case EOpcode::Read:        return "READ";
        case EOpcode::Write:       return "WRITE";
        case EOpcode::Binop:       return "BINOP " + instruction.op;
        case EOpcode::Label:       return "LABEL " + instruction.name;
        case EOpcode::Jmp:         return "JMP " + instruction.name;
        case EOpcode::CJmpZero:    return "CJMP z, " + instruction.name;
        case EOpcode::CJmpNotZero: return "CJMP nz, " + instruction.name;
        case EOpcode::End:         return "END";
    }

    throw interpret_error("Unknown stack machine instruction", instruction.position);
}

void Program::print(std::ostream& output) const {
    for (const auto& instruction : instructions_) {
        output << to_string(instruction) << '\n';
    }

    output.flush();
}

Program Program::read(std::istream& input) {
    Program program;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        std::istringstream source(line);
        std::string opcode;
        if (!(source >> opcode)) continue;

        Instruction instruction;
        instruction.position = Pos(line_number, 1);
        bool valid = true;
        if (opcode == "CONST") {
            instruction.opcode = EOpcode::Const;
            valid = static_cast<bool>(source >> instruction.value);
        } else if (opcode == "LD" || opcode == "ST" || opcode == "LABEL" || opcode == "JMP") {
            if (opcode == "LD") instruction.opcode = EOpcode::Ld;
            if (opcode == "ST") instruction.opcode = EOpcode::St;
            if (opcode == "LABEL") instruction.opcode = EOpcode::Label;
            if (opcode == "JMP") instruction.opcode = EOpcode::Jmp;
            valid = static_cast<bool>(source >> instruction.name);
        } else if (opcode == "BINOP") {
            instruction.opcode = EOpcode::Binop;
            valid = static_cast<bool>(source >> instruction.op);
            valid = valid && ValueOps::IsOperator(instruction.op);
        } else if (opcode == "CJMP") {
            std::string condition;
            valid = static_cast<bool>(source >> condition >> instruction.name);
            valid = valid && (condition == "z," || condition == "nz,");
            instruction.opcode = condition == "z," ? EOpcode::CJmpZero : EOpcode::CJmpNotZero;
        } else if (opcode == "READ") {
            instruction.opcode = EOpcode::Read;
        } else if (opcode == "WRITE") {
            instruction.opcode = EOpcode::Write;
        } else if (opcode == "END") {
            instruction.opcode = EOpcode::End;
        } else {
            valid = false;
        }

        std::string extra;
        if (!valid || (source >> extra)) {
            throw ast_error("Invalid stack machine instruction: " + line, instruction.position);
        }
        program.emit(std::move(instruction));
    }
    if (input.bad()) {
        throw ast_error("Cannot read stack machine program");
    }
    return program;
}

}
}
