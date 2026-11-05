#include "sm/machine/machine.h"

namespace NTomatoInterpretato {
namespace NSm {

Value StackMachine::pop(const Instruction& instruction) {
    if (stack_.empty()) {
        throw interpret_error(to_string(instruction) + ": stack is empty", instruction.position);
    }

    Value value = stack_.back();
    stack_.pop_back();

    return value;
}

std::unordered_map<std::string, std::size_t> StackMachine::collect_labels(const Program& program) const {
    std::unordered_map<std::string, std::size_t> labels;

    const auto& instructions = program.instructions();
    for (std::size_t i = 0; i < instructions.size(); ++i) {
        if (instructions[i].opcode != EOpcode::Label) {
            continue;
        }

        if (labels.contains(instructions[i].name)) {
            throw interpret_error("Duplicate label '" + instructions[i].name + "'");
        }

        labels[instructions[i].name] = i;
    }

    return labels;
}

bool StackMachine::run(const Program& program) {
    stack_.clear();
    variables_.clear();
    const auto labels = collect_labels(program);
    const auto& instructions = program.instructions();
    for (const auto& instruction : instructions) {
        if ((instruction.opcode == EOpcode::Jmp || instruction.opcode == EOpcode::Jz ||
             instruction.opcode == EOpcode::Jnz) && !labels.contains(instruction.name)) {
            throw interpret_error("Unknown label '" + instruction.name + "'", instruction.position);
        }
    }

    auto jump_to = [&](const Instruction& instruction) {
        auto it = labels.find(instruction.name);
        if (it == labels.end()) {
            throw interpret_error("Unknown label '" + instruction.name + "'", instruction.position);
        }
        return it->second;
    };

    std::size_t ip = 0;

    while (ip < instructions.size()) {
        const Instruction& instruction = instructions[ip];

        switch (instruction.opcode) {
            case EOpcode::Const:
                stack_.push_back(instruction.value);
                break;

            case EOpcode::Ld: {
                auto it = variables_.find(instruction.name);
                if (it == variables_.end()) {
                    throw interpret_error("Variable '" + instruction.name + "' is not defined", instruction.position);
                }
                stack_.push_back(it->second);
                break;
            }

            case EOpcode::St:
                variables_[instruction.name] = pop(instruction);
                break;

            case EOpcode::Read: {
                Value input;
                if (!(input_ >> input)) {
                    throw interpret_error("READ: expected an integer on input", instruction.position);
                }
                stack_.push_back(input);
                break;
            }

            case EOpcode::Write:
                output_ << pop(instruction) << '\n';
                break;

            case EOpcode::Binop: {
                Value rhs = pop(instruction);
                Value lhs = pop(instruction);
                stack_.push_back(ValueOps::Apply(instruction.op, lhs, rhs));
                break;
            }

            case EOpcode::Label:
                break;

            case EOpcode::Jmp:
                ip = jump_to(instruction);
                continue;

            case EOpcode::Jz:
                if (pop(instruction) == 0) {
                    ip = jump_to(instruction);
                    continue;
                }
                break;

            case EOpcode::Jnz:
                if (pop(instruction) != 0) {
                    ip = jump_to(instruction);
                    continue;
                }
                break;

            default:
                throw interpret_error("Unknown stack machine opcode", instruction.position);
        }

        ++ip;
    }

    output_.flush();

    return true;
}

}
}
