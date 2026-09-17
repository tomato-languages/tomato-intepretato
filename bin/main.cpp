#include <iostream>
#include <fstream>
#include <interpreter.h>
#include <argparser.h>

using namespace TomatoInterpretato;

int main(int argc, char** argv) {

    ArgumentParser::ArgParser parser("tomato-intepretato");

    parser.AddStringArgument('p', "path", "path to JSON AST. If omitted, the AST is read from stdin, followed by program input. (Positional)").Positional().Default("");
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
    std::string path = parser.GetStringValue("path");

    try {
        if (path.empty() || path == "-") {
            interpretator.interpret(std::cin);
        } else {
            std::ifstream fstream(path);
            if (!fstream) {
                std::cerr << "Error: cannot open file '" << path << "'" << std::endl;
                return 2;
            }
            interpretator.interpret(fstream);
        }
    } catch (const json_error& e) {
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
