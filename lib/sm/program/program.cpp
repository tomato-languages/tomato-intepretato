#include "sm/program/program.h"
#include <sstream>

namespace NTomatoInterpretato {
namespace NSm {

namespace {

std::string quote(const std::string& text) {
    std::string result = "\"";
    for (char ch : text) {
        switch (ch) {
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += ch;
        }
    }
    return result + '"';
}

bool read_quoted(std::istream& source, std::string& text) {
    char ch;
    if (!(source >> ch) || ch != '"') return false;
    while (source.get(ch)) {
        if (ch == '"') return true;
        if (ch != '\\') {
            text += ch;
            continue;
        }
        if (!source.get(ch)) return false;
        switch (ch) {
            case '\\': text += '\\'; break;
            case '"':  text += '"'; break;
            case 'n':  text += '\n'; break;
            case 'r':  text += '\r'; break;
            case 't':  text += '\t'; break;
            default:   return false;
        }
    }
    return false;
}

}

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
        case EOpcode::Jz:          return "JZ " + instruction.name;
        case EOpcode::Jnz:         return "JNZ " + instruction.name;
        case EOpcode::Array:       return "ARRAY " + std::to_string(instruction.value);
        case EOpcode::String:      return "STRING " + quote(instruction.text);
        case EOpcode::Length:      return "LENGTH";
        case EOpcode::Elem:        return "ELEM";
        case EOpcode::Set:         return "SET";
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
        } else if (opcode == "LD" || opcode == "ST" || opcode == "LABEL" || opcode == "JMP" ||
                   opcode == "JZ" || opcode == "JNZ") {
            if (opcode == "LD") instruction.opcode = EOpcode::Ld;
            if (opcode == "ST") instruction.opcode = EOpcode::St;
            if (opcode == "LABEL") instruction.opcode = EOpcode::Label;
            if (opcode == "JMP") instruction.opcode = EOpcode::Jmp;
            if (opcode == "JZ") instruction.opcode = EOpcode::Jz;
            if (opcode == "JNZ") instruction.opcode = EOpcode::Jnz;
            valid = static_cast<bool>(source >> instruction.name);
        } else if (opcode == "BINOP") {
            instruction.opcode = EOpcode::Binop;
            valid = static_cast<bool>(source >> instruction.op);
            valid = valid && ValueOps::IsOperator(instruction.op);
        } else if (opcode == "ARRAY") {
            instruction.opcode = EOpcode::Array;
            valid = source >> instruction.value && instruction.value >= 0;
        } else if (opcode == "STRING") {
            instruction.opcode = EOpcode::String;
            valid = read_quoted(source, instruction.text);
        } else if (opcode == "LENGTH") {
            instruction.opcode = EOpcode::Length;
        } else if (opcode == "ELEM") {
            instruction.opcode = EOpcode::Elem;
        } else if (opcode == "SET") {
            instruction.opcode = EOpcode::Set;
        } else if (opcode == "READ") {
            instruction.opcode = EOpcode::Read;
        } else if (opcode == "WRITE") {
            instruction.opcode = EOpcode::Write;
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
