#pragma once
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "ast/common/value.h"
#include "common/errors.h"
#include "sm/program/program.h"

namespace NTomatoInterpretato {
namespace NSm {

class StackMachine {
public:

    StackMachine(std::istream& input = std::cin, std::ostream& output = std::cout)
        : input_(input)
        , output_(output)
    {}

    bool run(const Program& program);

private:

    std::istream& input_;
    std::ostream& output_;

    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> variables_;

    Value pop(const Instruction& instruction);
    std::unordered_map<std::string, std::size_t> collect_labels(const Program& program) const;

};

}
}
