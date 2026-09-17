#include "interpreter.h"
#include "common/utils.h"

bool interpret(std::istream& ast_input, std::istream& input, std::ostream& output) {
    NTomatoInterpretato::Interpreter interpretator(input, output);
    auto builder = NTomatoInterpretato::CreateAstBuilder(
        NTomatoInterpretato::BuilderSettings{
            .mode = NTomatoInterpretato::EBuilderMode::JSON
        },
        ast_input
    );
    return interpretator.interpret(builder);
}
