#pragma once
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include "ast/common/value.h"
#include "common/errors.h"

namespace NTomatoInterpretato {
namespace NSm {

enum class EOpcode {
    Const,
    Ld,
    St,
    Read,
    Write,
    Binop,
    Label,
    Jmp,
    Jz,
    Jnz
};

struct Instruction {
    EOpcode opcode = EOpcode::Const;

    Value value = 0;
    std::string name;
    std::string op;

    Pos position;
};

std::string to_string(const Instruction& instruction);

class Program {
public:

    void emit(Instruction&& instruction) {
        instructions_.push_back(std::move(instruction));
    }

    void emit_const(Value value, Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::Const, .value = value, .position = position});
    }

    void emit_load(const std::string& name, Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::Ld, .name = name, .position = position});
    }

    void emit_store(const std::string& name, Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::St, .name = name, .position = position});
    }

    void emit_read(Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::Read, .position = position});
    }

    void emit_write(Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::Write, .position = position});
    }

    void emit_binop(const std::string& op, Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::Binop, .op = op, .position = position});
    }

    void emit_label(const std::string& label) {
        emit(Instruction{.opcode = EOpcode::Label, .name = label});
    }

    void emit_jump(const std::string& label) {
        emit(Instruction{.opcode = EOpcode::Jmp, .name = label});
    }

    void emit_jump_if_zero(const std::string& label, Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::Jz, .name = label, .position = position});
    }

    void emit_jump_if_not_zero(const std::string& label, Pos position = Pos()) {
        emit(Instruction{.opcode = EOpcode::Jnz, .name = label, .position = position});
    }

    std::string new_label() {
        return "L" + std::to_string(next_label_++);
    }

    const std::vector<Instruction>& instructions() const {
        return instructions_;
    }

    std::size_t size() const {
        return instructions_.size();
    }

    void print(std::ostream& output) const;
    static Program read(std::istream& input);

private:

    std::vector<Instruction> instructions_;
    std::size_t next_label_ = 1;

};

}
}
