#ifndef LEXER_H
#define LEXER_H

#include <QString>
#include <QVector>
#include <QRegularExpression>

enum TokenType {
    TOKEN_ID, //для имен переменных
    TOKEN_NUMBER, //для числовых литералов (10, 3.14)
    TOKEN_STRING, //для строковых литералов
    TOKEN_KEYWORD, //if, else, def
    TOKEN_OP, //+, -, =, ==
    TOKEN_NEWLINE, //\n
    TOKEN_INDENT, //отступы
    TOKEN_EOF //конец файла
};

struct Token
{
    TokenType type;
    QString value;
    int line; //Строка, где начинается токен
    // int column; //Столбец, где начинается токен
    Token(TokenType type, QString value, int line) : type(type), value(value), line(line) {}
};

class Lexer
{
public:
    Lexer(const QString& code);
    QVector<Token> tokenize(); //Главный метод

private:
    QString code;
    int pos = 0; //текушая позиция в коде
    int line = 1; //текущая строка
    int column = 1; //текущая колонка
    QVector<QString> convenientDemoTokenTypes = {"TOKEN_ID", "TOKEN_NUMBER", "TOKEN_STRING", "TOKEN_KEYWORD", "TOKEN_OP", "TOKEN_NEWLINE", "TOKEN_INDENT", "TOKEN_EOF"};

    Token nextToken(); //Следующий токен
    Token readNumber(); //Читает число
    Token readString(); //Читает строку
    Token readIdentifier(); //Читает имя или ключевое слово
    Token readOperator(); //Читает оператор(+, -, =)
    void skipWhitespace(); //Пропускает пробелы, но не \n
    void skipComment(); //Пропускает комментарии (#...)
};

#endif // LEXER_H
