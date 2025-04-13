#include "Parser.h"


Parser::Parser(const QVector<Token>& tokens) : tokens(tokens) {}

std::shared_ptr<ASTNode> Parser::parse() { return parseExpression(); }

std::shared_ptr<ASTNode> Parser::parseExpression()
{
    auto left = parseTerm();
    while (peek().type == TOKEN_OP && (peek().value == "+" || peek().value == "-"))
    {
        QString op = advance().value;
        auto right = parseTerm();
        left = std::make_shared<BinOpNode>(left, op, right);
    }
    return left;
}

std::shared_ptr<ASTNode> Parser::parseTerm()
{
    auto left = parsePower();
    while (peek().type == TOKEN_OP && (peek().value == "*" || peek().value == "/" || peek().value == "//" || peek().value == "%"))
    {
        QString op = advance().value;
        auto right = parsePower();
        left = std::make_shared<BinOpNode>(left, op, right);
    }
    return left;
}

std::shared_ptr<ASTNode> Parser::parsePower()
{
    auto left = parseFactor();
    if (peek().type == TOKEN_OP && peek().value == "**")
    {
        QString op = advance().value;
        auto right = parsePower();
        return std::make_shared<BinOpNode>(left, op, right);
    }
    return left;
}

std::shared_ptr<ASTNode> Parser::parseFactor()
{
    Token token = peek();
    if (token.type == TOKEN_EOF)
    {
        return nullptr;
    }

    if (token.type == TOKEN_NUMBER)
    {
        advance();
        auto num = std::make_shared<NumberNode>();
        num->value = token.value.toDouble();
        return num;
    }
    else if (token.type == TOKEN_OP && token.value == "(")
    {
        advance();
        auto expr = parseExpression();
        if (peek().type == TOKEN_OP || peek().value == ")")
        {
            advance();
        }
        else
        {
            throw std::runtime_error("Expected ')'");
        }
        return expr;
    }
    throw std::runtime_error("Unexpected token" + token.value.toStdString());
}

Token Parser::peek() const { return (current < tokens.size()) ? tokens[current] : Token(TOKEN_EOF, "", 0); }

Token Parser::advance() { return (current < tokens.size()) ? tokens[current++] : Token(TOKEN_EOF, "", 0); }
