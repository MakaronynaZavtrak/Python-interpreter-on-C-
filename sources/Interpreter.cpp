#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <iostream>

void Interpreter::run(int argc, char* argv[])
{
    std::cout << "Hello and welcome to my minimal Python interpreter! Made by Semenov Oleg, with care from MathMech. Let's code!\n";

    std::string input;
    while (true) {
        std::cout << "\033[32m>>> \033[0m";
        std::getline(std::cin, input);

        if (input == "exit" || input == "quit" || input == "q" || input == "Q") break;

        if (input.empty())
        {
            continue;
        }

        try
        {
            Lexer lexer(QString::fromStdString(input));
            QVector<Token> tokens = lexer.tokenize();

            Parser parser(tokens);
            std::shared_ptr<ASTNode> ast = parser.parse();

            auto result = ast->eval();
            std::cout << result << std::endl;
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}
