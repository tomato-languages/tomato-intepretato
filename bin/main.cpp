#include <iostream>
#include <sstream>
#include <fstream>
#include <interpreter.h>
#include <argparser.h>

using namespace TomatoInterpretato;

int main(int argc, char** argv) {

    ArgumentParser::ArgParser parser("IS");
    Interpreter interpretator;

    parser.AddStringArgument('p', "path", "path to file for interpretate. (Positional)").Positional().Default("");
    parser.AddHelp('h', "help", "TomatoInterpretato Interpretator");

    if (!parser.Parse(argc, argv)) {
        std::cerr << "Error parsing arguments: " << std::endl;
        std::cout << parser.HelpDescription() << std::endl;

        return 1;
    }

    if (parser.Help()) {
        std::cout << parser.HelpDescription() << std::endl;
        return 0;
    }

    if (argc == 1) {

        std::string input_str;

        while (std::getline(std::cin, input_str)) {

            std::stringstream input_str_stream(input_str);

            try {
                interpretator.interpret(input_str_stream);
            } catch (const interpret_error& e) {
                std::cerr << "Interpret error: " << e.what() << std::endl;
            } catch (const parsing_error& e) {
                std::cerr << "Parsing error: " << e.what() << std::endl;
            } catch (const lexer_error& e) {
                std::cerr << "Lexer error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Unexpected exception: " << e.what() << std::endl;
            }
        }


    } else {

        if (!parser.GetStringValue("path").ends_with(".is")) {
            std::cerr << "Error: file doesn't .is extension." << std::endl;
            std::cout << parser.HelpDescription() << std::endl;
            return 1;
        }

        std::string path = parser.GetStringValue("path");

        std::ifstream fstream(path);

        try {
            interpretator.interpret(fstream);
        } catch (const interpret_error& e) {
            std::cerr << "Interpret error: " << e.what() << std::endl;
        } catch (const parsing_error& e) {
            std::cerr << "Parsing error: " << e.what() << std::endl;
        } catch (const lexer_error& e) {
            std::cerr << "Lexer error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
        }
    }

    return 0;
}