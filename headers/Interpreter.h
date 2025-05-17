#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <qlist.h>
#include <QString>

/**
 * @class Interpreter
 * @brief Минимальный интерпретатор Python, предоставляющий REPL для выполнения Python-подобного кода.
 *
 * Класс Interpreter служит основной точкой входа для запуска минимального интерпретатора Python.
 * Этот интерпретатор поддерживает чтение пользовательского ввода, токенизацию, разбор в абстрактное синтаксическое дерево (AST)
 * и выполнение Python-подобных инструкций и выражений. REPL позволяет пользователю интерактивно вводить код
 * и мгновенно видеть результаты.
 */
class Interpreter {
public:
    void run(int argc, char* argv[]);
private:
    QVector<int> indentStack;
    bool inBlock = false;
    const std::string PROMPT_MAIN = "\033[32m>>> \033[0m";
    const std::string PROMPT_CONTINUE = "\033[32m... \033[0m";
    const int INDENT_SIZE = 4;

    bool isBlockStatement(const QString&);
    int getIndentLevel(const QString&);
};

#endif // INTERPRETER_H