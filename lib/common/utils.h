#pragma once
#include <istream>
#include <memory>
#include "ast/ast_builders/ast_builder.h"

namespace NTomatoInterpretato {

enum class EBuilderMode {
    UNKNOWN = 0,
    SCRIPT = 1,
    JSON = 2,
};

struct BuilderSettings {
    EBuilderMode mode = EBuilderMode::UNKNOWN;
};

std::shared_ptr<NAst::IAstBuilder> CreateAstBuilder(const BuilderSettings& settings, std::istream& istream);

}
