#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include <memory>
#include <cmath>

class ASTNode
{
public:
    virtual ~ASTNode() = default;
    virtual double eval() const = 0;
    virtual QString toString() const = 0;
};

class NumberNode : public ASTNode
{
public:
    double value;
    QString toString() const override { return QString::number(value); }
    double eval() const override { return value; }
};

class BinOpNode : public ASTNode
{
public:
    BinOpNode(std::shared_ptr<ASTNode> left, const QString& op, std::shared_ptr<ASTNode> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

    QString toString() const override
    {
        return "(" + left->toString() + " " + op + " " + right->toString() + ")";
    }

    double eval() const override
    {
        double l = left->eval();
        double r = right->eval();

        if (op == "+") return l + r;
        if (op == "-") return l - r;
        if (op == "*") return l * r;
        if (op == "**") return pow(l, r);
        if (op == "/") return r != 0 ? l / r : throw std::runtime_error("Division by zero");
        if (op == "%") return r != 0 ? int(l) % int(r) : throw std::runtime_error("Division by zero");
        if (op == "//") return r != 0 ? int(l / r) : throw std::runtime_error("Division by zero");
        if (op == "==") return l == r;
        if (op == "!=") return l != r;

        throw std::runtime_error("Unknown operator: " + op.toStdString());
    }

    std::shared_ptr<ASTNode> left;
    QString op; // "+", "-", "=", "/", "%", "*", "**", "//", "=="
    std::shared_ptr<ASTNode> right;
};

class VarNode : public ASTNode
{
public:
    QString name;
    QString toString() const override
    {
        return name;
    }
};

class Parser
{
public:
    Parser(const QVector<Token> &tokens);
    std::shared_ptr<ASTNode> parse(); //Главный метод

private:
    QVector<Token> tokens;
    int current = 0;

    std::shared_ptr<ASTNode> parseExpression();
    std::shared_ptr<ASTNode> parseTerm();
    std::shared_ptr<ASTNode> parseFactor();
    Token peek() const;
    Token advance();
};
#endif // PARSER_H
