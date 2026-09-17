#include <iostream>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "interpreter/interpreter.h"
#include "common/utils.h"
#include "common/errors.h"
#include "argparser.h"

using namespace NTomatoInterpretato;

inline EBuilderMode DetermineBuilderMode(ArgumentParser::ArgParser& parser) {
    if (parser.GetFlag('j')) {
        return EBuilderMode::JSON;
    }
    if (parser.GetFlag('s')) {
        return EBuilderMode::SCRIPT;
    }
    return EBuilderMode::UNKNOWN;
}

inline std::istream& DetermineIStream(ArgumentParser::ArgParser& parser, std::ifstream& file) {
    const std::string path = parser.GetStringValue("path");

    if (path.empty() || path == "-") {
        return std::cin;
    }

    file.open(path);
    if (!file) {
        throw interpret_error("cannot open file '" + path + "'");
    }
    return file;
}

int main(int argc, char** argv) {

    ArgumentParser::ArgParser parser("tomato-intepretato");

    parser.AddStringArgument('p', "path", "path to target file. If omitted, the AST is read from stdin, followed by program input. (Positional)").Positional().Default("");
    parser.AddFlag('j', "json", "interpret script from JSON AST file");
    parser.AddFlag('s', "script", "interpret script from .is file");
    parser.AddHelp('h', "help", "TomatoInterpretato: interpreter of the JSON AST representation");

    if (!parser.Parse(argc, argv)) {
        std::cerr << "Error parsing arguments" << std::endl;
        std::cout << parser.HelpDescription() << std::endl;
        return 2;
    }

    if (parser.Help()) {
        std::cout << parser.HelpDescription() << std::endl;
        return 0;
    }

    std::ios::sync_with_stdio(false);

    Interpreter interpretator(std::cin, std::cout);

    try {
        std::ifstream file;
        std::shared_ptr<NAst::IAstBuilder> builder = CreateAstBuilder(
            BuilderSettings{
                .mode = DetermineBuilderMode(parser)
            },
            DetermineIStream(parser, file)
        );

        if (builder == nullptr) [[unlikely]] {
            std::cerr << "no relevant builder" << std::endl;
            return 1;
        }

        interpretator.interpret(builder);
    } catch (const nlohmann::json::exception& e) {
        std::cout.flush();
        std::cerr << "JSON error: " << e.what() << std::endl;
        return 1;
    } catch (const ast_error& e) {
        std::cout.flush();
        std::cerr << "AST error: " << e.what() << std::endl;
        return 1;
    } catch (const interpret_error& e) {
        std::cout.flush();
        std::cerr << "Interpret error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cout.flush();
        std::cerr << "Unexpected exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
