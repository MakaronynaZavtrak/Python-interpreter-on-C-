#include "Lexer.h"
// #include <iostream>
// #include "qdebug.h"

Lexer::Lexer(const QString& code) : code(code) {}

QVector<Token> Lexer::tokenize() {
    QVector<Token> tokens;
    while (true) {
        Token token = nextToken();
        // std::cout << "Token:" << token.value.toStdString() << "Type:" << convenientDemoTokenTypes[token.type].toStdString() << "Pos:" << pos;

        if (token.type == TOKEN_EOF)
        {
            break;
        }

        if (!token.value.isEmpty())
        {
            tokens.append(token);
        }
    }
    return tokens;
}

Token Lexer::nextToken()
{
    skipWhitespace();
    skipComment();

    if (pos >= code.length())
    {
        return {TOKEN_EOF, "", line};
    }

    QChar ch = code[pos];

    if (ch.isDigit())
    {
        return readNumber();
    }
    else if (ch == '\"' || ch == '\'')
    {
        return readString();
    }
    else if (ch.isLetter() || ch == '_')
    {
        return readIdentifier();
    }
    else
    {
        return readOperator();
    }
}

Token Lexer::readNumber()
{
    int start = pos;
    while (pos < code.length() && (code[pos].isDigit() || code[pos] == '.'))
    {
        pos++;
    }
    QString num = code.mid(start, pos - start);
    return {TOKEN_NUMBER, num, line};
}

Token Lexer::readString()
{
    QChar quote = code[pos];
    pos++;
    column++;
    int start = pos;

    while (pos < code.length() && code[pos] != quote)
    {
        if (code[pos] == '\n')
        {
            line++;
            column = 1;
        }
        pos++;
        column++;
    }

    if (pos >= code.length())
    {
        throw std::runtime_error("Unterminated string literal");
    }

    QString str = code.mid(start, pos - start);
    pos++;
    column++;
    return {TOKEN_STRING, str, line};
}

Token Lexer::readIdentifier()
{
    int start = pos;
    while (pos < code.length() && (code[pos].isLetter() || code[pos] == '_'))
    {
        pos++;
    }
    QString id = code.mid(start, pos - start);
    if (id == "if" || id == "else" || id == "def") {
        return {TOKEN_KEYWORD, id, line};
    }
    return {TOKEN_ID, id, line};
}

Token Lexer::readOperator()
{
    QChar op = code[pos++];

    if (pos < code.length())
    {
        QChar next = code[pos];
        QString combined = QString(op) + next;
        if (combined == "==" || combined == "+=" || combined == "!=" || combined == "-=" || combined == "//" || combined == "**")
        {
            pos++;
            return {TOKEN_OP, combined, line};
        }
    }

    return {TOKEN_OP, QString(op), line};
}

void Lexer::skipWhitespace()
{
    while (pos < code.length() && code[pos].isSpace() && code[pos] != '\n')
    {
        pos++;
        column++;
    }
}

void Lexer::skipComment()
{
    if (pos < code.length() && code[pos] == '#')
    {
        while (pos < code.length() && code[pos] != '\n')
        {
            pos++;
        }
        column = 1;
        line++;
        pos++;
    }
}
