#include "common/utils.h"
#include "ast/ast_builders/json_builder/ast_builder.h"
#include "common/errors.h"

#include <memory>

namespace NTomatoInterpretato {

namespace {

NAst::JsonAstBuilder::Json CreateJsonData(std::istream& istream) {
    if (!istream) {
        throw interpret_error("Input stream is not valid");
    }

    NAst::JsonAstBuilder::Json json;
    istream >> json;
    return json;
}

}

std::shared_ptr<NAst::IAstBuilder> CreateAstBuilder(const BuilderSettings& settings, std::istream& istream) {
    switch (settings.mode) {
        case EBuilderMode::JSON:
            return std::make_shared<NAst::JsonAstBuilder>(CreateJsonData(istream));
        case EBuilderMode::SCRIPT:
            return nullptr;
        default:
            break;
    }
    return nullptr;
}

}
