#include "Interpreter.h"

int main(const int argc, char *argv[])
{
    Interpreter interpreter;
    interpreter.run(argc, argv);
    return 0;
}
