#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include "ast/ast_builders/json_builder/ast_builder.h"
#include "ast/ast_builders/script_builder/ast_builder.h"
#include "interpreter/interpreter.h"
#include "sm/compiler/compiler.h"
#include "sm/machine/machine.h"
#include "argparser.h"

using namespace NTomatoInterpretato;

NAst::AST BuildAst(const std::string& source, bool script, std::istream& input, std::ostream& output) {
    if (script) {
        std::istringstream stream(source);
        return NAst::ScriptAstBuilder(input, output).build(stream);
    }
    return NAst::JsonAstBuilder(input, output).build(nlohmann::json::parse(source));
}

struct RunResult {
    std::string output;
    std::string error;
};

RunResult Run(const std::string& source, bool script, const std::string& input, bool machine) {
    std::istringstream in(input);
    std::ostringstream out;
    RunResult result;
    try {
        auto ast = BuildAst(source, script, in, out);
        if (machine) {
            auto program = NSm::Compiler().compile(ast);
            NSm::StackMachine(in, out).run(program);
        } else {
            Interpreter().interpret(ast);
        }
    } catch (const std::exception& error) {
        result.error = error.what();
    }
    result.output = out.str();
    return result;
}

int main(int argc, char** argv) {
    ArgumentParser::ArgParser parser("tomato-intepretato");
    parser.AddStringArgument('p', "path", "path to JSON AST, source text or SM program (positional); stdin if omitted")
        .Positional().Default("");
    parser.AddFlag('s', "script", "read source text instead of JSON AST");
    parser.AddFlag('i', "interpret", "interpret AST");
    parser.AddFlag('c', "compile", "compile AST to SM");
    parser.AddFlag('m', "machine", "compile AST and run SM");
    parser.AddFlag('x', "execute", "run a saved SM program");
    parser.AddFlag('k', "check", "compare AST and SM execution");
    parser.AddHelp('h', "help", "TomatoInterpretato: JSON AST interpreter, compiler and stack machine");

    if (!parser.Parse(argc, argv)) {
        std::cerr << "Error parsing arguments\n";
        std::cerr << parser.HelpDescription() << '\n';
        return 2;
    }
    if (parser.Help()) {
        std::cout << parser.HelpDescription() << '\n';
        return 0;
    }

    std::string mode;
    for (char flag : {'i', 'c', 'm', 'x', 'k'}) {
        if (parser.GetFlag(flag)) {
            if (!mode.empty()) {
                std::cerr << "Choose one execution mode\n";
                return 2;
            }
            mode = std::string("-") + flag;
        }
    }
    if (mode.empty()) {
        std::cerr << "Choose one execution mode: -i, -c, -m, -x or -k\n";
        return 2;
    }
    const bool script = parser.GetFlag('s');
    if (script && mode == "-x") {
        std::cerr << "Script input cannot be used with SM execution mode\n";
        return 2;
    }
    const std::string path = parser.GetStringValue("path");

    try {
        std::ifstream file;
        std::istream* source = &std::cin;
        if (!path.empty() && path != "-") {
            file.open(path);
            if (!file) throw interpret_error("Cannot open file: " + path);
            source = &file;
        }
        if (mode == "-x") {
            auto program = NSm::Program::read(*source);
            NSm::StackMachine(std::cin, std::cout).run(program);
            return 0;
        }

        std::string text;
        if (script) {
            text.assign(std::istreambuf_iterator<char>(*source), {});
        } else {
            nlohmann::json json;
            *source >> json;
            text = json.dump();
        }
        if (mode == "-k") {
            std::string input{std::istreambuf_iterator<char>(std::cin), {}};
            auto tree = Run(text, script, input, false);
            auto machine = Run(text, script, input, true);
            std::cout << tree.output;
            if (!tree.error.empty()) std::cerr << "AST error: " << tree.error << '\n';
            if (!machine.error.empty()) std::cerr << "SM error: " << machine.error << '\n';
            bool same = tree.output == machine.output && tree.error.empty() == machine.error.empty();
            std::cerr << (same ? "check: results agree\n" : "check: results differ\n");
            return same && tree.error.empty() ? 0 : 1;
        }

        auto ast = BuildAst(text, script, std::cin, std::cout);
        if (mode == "-i") {
            Interpreter().interpret(ast);
        } else {
            auto program = NSm::Compiler().compile(ast);
            if (mode == "-c") program.print(std::cout);
            else NSm::StackMachine(std::cin, std::cout).run(program);
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
