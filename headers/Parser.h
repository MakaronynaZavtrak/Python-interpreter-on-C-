#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Value.h"
#include "Environment.h"
#include <memory>
#include <cmath>
#include <utility>

/**
 * @class ASTNode
 * @brief Представляет узел абстрактного синтаксического дерева (AST) в интерпретаторе языка программирования.
 *
 * ASTNode служит базовым классом для всех типов узлов в абстрактном синтаксическом дереве.
 * Производные классы реализуют конкретные типы узлов и определяют их поведение для вычисления
 * и строкового представления. Этот класс спроектирован для расширения и не может быть создан напрямую.
 *
 * @details
 * Абстрактное синтаксическое дерево представляет иерархическую структуру исходного кода, где каждый
 * узел соответствует синтаксической конструкции, такой как переменная, оператор или функция.
 * От производных классов ожидается предоставление конкретных реализаций методов `eval` и `toString`.
 */
class ASTNode {
public:
    virtual ~ASTNode() = default;

    virtual Value eval(Environment &env) const = 0;

    [[nodiscard]] virtual QString toString() const = 0;
};

/**
 * @class ValueNode
 * @brief Представляет узел, содержащий значение, в абстрактном синтаксическом дереве.
 *
 * ValueNode хранит конкретное значение, представленное экземпляром класса `Value`.
 * Этот узел используется для представления литеральных значений, таких как числа, строки, булевы значения и другие.
 *
 * @details
 * Узел предоставляет методы для вычисления (`eval`) и строкового представления (`toString`) своего значения.
 * Метод `eval` просто возвращает сохранённое значение, в то время как `toString` преобразует его в строку
 * на основе типа данных, содержащегося в `Value`. Класс является финальным и не может быть унаследован.
 */
class ValueNode final : public ASTNode {
public:
    explicit ValueNode(Value value) : value(std::move(value)) {}

    Value value;

    /**
     * @brief Возвращает строковое представление значения узла.
     *
     * Метод преобразует значение узла в строку на основе типа данных, содержащегося в `Value`.
     * Поддерживаемые типы данных включают `double`, `int`, `QString` и `bool`. Для типов данных,
     * которые не поддерживаются, возвращается строка "<Unknown type of value>".
     *
     * @return Строковое представление значения узла.
     */
    [[nodiscard]] QString toString() const override {
        const auto& data = value.data;
        
        if (std::holds_alternative<double>(data)) {
            return QString::number(std::get<double>(data));
        }
        if (std::holds_alternative<int>(data)) {
            return QString::number(std::get<int>(data));
        }
        if (std::holds_alternative<QString>(data)) {
            return "\'" + std::get<QString>(data) + "\'";
        }
        if (std::holds_alternative<bool>(data)) {
            return std::get<bool>(data) ? "True" : "False";
        }
        return "<Unknown type of value>";
    }

    Value eval(Environment &env) const override { return {value}; }
};

/**
 * @class BinOpNode
 * @brief Представляет узел бинарной операции в абстрактном синтаксическом дереве (AST) интерпретатора языка программирования.
 *
 * BinOpNode обрабатывает бинарные операции, такие как арифметические выражения, сравнения или конкатенация строк.
 * Он принимает два дочерних узла (левый и правый) и оператор, и вычисляет их для получения результата.
 *
 * @details
 * Класс поддерживает вычисление (`eval`) и строковое представление (`toString`) бинарных операций.
 * Поддерживаемые операции включают арифметические (`+`, `-`, `*`, `/`, `%`, `**`, `//`), сравнения (`==`, `!=`, `>`, `>=`, `<`, `<=`),
 * и строковые операции. Узел вычисляется путем рекурсивного вычисления левого и правого дочерних узлов
 * и применения указанного оператора. Если встречается неподдерживаемая операция или комбинация типов операндов,
 * генерируются соответствующие ошибки.
 *
 * Метод `eval` обеспечивает правильную обработку типов (например, числовых, строковых или смешанных) и делегирует конкретные
 * поведения вспомогательным методам, таким как `evalTwoNumbers`, `evalTwoStrings` и `evalNumAndString`.
 */
class BinOpNode final : public ASTNode {
public:
    BinOpNode(std::shared_ptr<ASTNode> left, QString op, std::shared_ptr<ASTNode> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {
    }

    [[nodiscard]] QString toString() const override {
        return "(" + left->toString() + " " + op + " " + right->toString() + ")";
    }

    /**
     * @brief Вычисляет бинарную операцию, представленную текущим узлом.
     *
     * Этот метод рекурсивно вычисляет левый и правый дочерние узлы для получения их значений,
     * а затем применяет бинарную операцию, связанную с этим узлом (например, арифметическую, сравнение),
     * на основе строки оператора. Метод поддерживает операции над числовыми, строковыми или смешанными типами.
     * Выбрасывает исключение, если операция не поддерживается для данных типов операндов.
     *
     * @param env Среда выполнения, содержащая привязки переменных и состояние.
     * @return Результат вычисления бинарной операции. Тип результата зависит от операции
     *         и операндов (например, числовой результат для арифметических операций, строковый результат для строковых операций).
     * @throws std::runtime_error Если операция не поддерживается для данных типов операндов.
     */
    Value eval(Environment &env) const override {
        const Value l = left->eval(env);
        const Value r = right->eval(env);

        if (
            (std::holds_alternative<double>(l.data) || std::holds_alternative<int>(l.data))
            &&
            (std::holds_alternative<double>(r.data) || std::holds_alternative<int>(r.data))
        ) {
            return evalTwoNumbers(l, r);
        }
        if (
            (std::holds_alternative<QString>(l.data) || std::holds_alternative<int>(l.data))
            &&
            (std::holds_alternative<QString>(r.data) || std::holds_alternative<int>(r.data))
        ) {
            return std::holds_alternative<QString>(l.data) && std::holds_alternative<QString>(r.data)
                       ? evalTwoStrings(l, r)
                       : evalNumAndString(l, r);
        }

        throw std::runtime_error("Unsupported operation: " + op.toStdString() + " in eval\n");
    }

    /**
     * @enum Operation
     * @brief Представляет операции, которые могут быть выполнены в бинарном узле AST.
     *
     * Перечисление `Operation` определяет набор поддерживаемых бинарных операций,
     * таких как арифметические операции, операции сравнения и операции модификации.
     * Оно используется для представления и обработки этих операций в синтаксическом анализе
     * и вычислении узлов AST.
     *
     * @details
     * Операции разбиваются на несколько категорий:
     * - **Арифметические операции**: Add, Subtract, Multiply, Divide, Modulo, IntDivide, Power.
     * - **Операции сравнения**: Equal, NotEqual, Greater, GreaterEqual, Less, LessEqual.
     * Эти операции могут быть применены к числовым значениям, а также могут включать
     * дополнительную проверку на деление на ноль или другие ошибки выполнения.
     */
    enum class Operation {
        Add, Subtract, Multiply, Power, Divide, Modulo, IntDivide,
        Equal, NotEqual, Greater, GreaterEqual, Less, LessEqual
    };

    /**
     * @brief Вычисляет бинарную операцию между двумя числовыми значениями.
     *
     * Этот метод вычисляет результат бинарной операции на основе предоставленных левого и правого операндов.
     * Он поддерживает арифметические операции, такие как сложение, вычитание, умножение, деление,
     * а также операции сравнения, такие как равенство, неравенство и реляционные сравнения.
     * Выполняются дополнительные проверки для операций, которые могут привести к ошибкам времени выполнения,
     * таких как деление или взятие остатка от деления на ноль.
     *
     * @param l Левый операнд бинарной операции.
     * @param r Правый операнд бинарной операции.
     * @return Результат вычисленной операции, инкапсулированный как `Value`. Точная природа результата
     *         (числовой, логический и т.д.) зависит от выполненной операции.
     * @throws std::runtime_error Если операция не поддерживается или недопустима, или если происходит ошибка времени выполнения,
     *                           такая как деление на ноль.
     */
    [[nodiscard]] Value evalTwoNumbers(const Value &l, const Value &r) const {
        const double lv = getNumericValue(l);
        const double rv = getNumericValue(r);

        switch (const Operation currentOp = parseOperation(op)) {
            case Operation::Add: return Value(lv + rv);
            case Operation::Subtract: return Value(lv - rv);
            case Operation::Multiply: return Value(lv * rv);
            case Operation::Power: return Value(pow(lv, rv));

            case Operation::Divide:
            case Operation::Modulo:
            case Operation::IntDivide:
                checkDivisionByZero(rv);
                if (currentOp == Operation::Divide) return Value(lv / rv);
                if (currentOp == Operation::Modulo) return Value(static_cast<int>(lv) % static_cast<int>(rv));
                return Value(static_cast<int>(lv / rv));

            case Operation::Equal: return Value(lv == rv);
            case Operation::NotEqual: return Value(lv != rv);
            case Operation::Greater: return Value(lv > rv);
            case Operation::GreaterEqual: return Value(lv >= rv);
            case Operation::Less: return Value(lv < rv);
            case Operation::LessEqual: return Value(lv <= rv);
            default: throw std::runtime_error("Unsupported operation: " + op.toStdString() + " in evalTwoNumbers");
        }
    }

    /**
     * @brief Извлекает числовое значение из объекта `Value`.
     *
     * Этот метод проверяет внутренние данные объекта `Value`. Если они содержат значение типа `double`,
     * оно возвращается непосредственно. Если данные имеют тип `int`, метод преобразует их в `double`
     * и возвращает результаты. Это удобно для унификации работы с числовыми значениями, особенно
     * при выполнении операций, требующих поддержки плавающей точки.
     *
     * @param v Объект класса `Value`, содержащий данные, из которых нужно извлечь числовое значение.
     * @return Значение типа `double`, извлеченное из объекта `Value`. Если данные имеют тип `int`,
     *         оно будет преобразовано в `double`.
     * @throws std::bad_variant_access Если данные объекта `Value` не содержат значения допустимого типа
     *                                 (`int` или `double`).
     */
private:
    [[nodiscard]] static double getNumericValue(const Value &v) {
        return std::holds_alternative<double>(v.data)
        ? std::get<double>(v.data)
        : std::get<int>(v.data);
    }

    /**
     * @brief Проверяет, делится ли число на ноль.
     *
     * Данный метод используется для проверки делителя на равенство нулю
     * с целью предотвращения ошибки деления на ноль, что может привести
     * к сбою выполнения программы.
     *
     * @param denominator Делитель, который проверяется на равенство нулю.
     * @throws std::invalid_argument Если делитель равен нулю.
     */
    static void checkDivisionByZero(const double denominator) {
        if (denominator == 0) {
            throw std::runtime_error("Division by zero");
        }
    }

    /**
     * @brief Разбирает строковое представление операции и сопоставляет его с соответствующим значением перечисления `Operation`.
     *
     * Этот метод принимает строку, представляющую математический оператор или оператор сравнения, и,
     * используя предопределенное отображение, преобразует её в соответствующее значение перечисления `Operation`.
     * Если предоставленная строка не соответствует ни одному известному оператору, выбрасывается исключение.
     *
     * @param op Строковое представление оператора для разбора (например, "+", "-", "**").
     * @return Соответствующее значение перечисления `Operation` для указанного оператора.
     * @throws std::runtime_error Если входная строка не соответствует поддерживаемой операции.
     */
    static Operation parseOperation(const QString &op) {
        static const std::unordered_map<QString, Operation> opMap = {
            {"+", Operation::Add}, {"-", Operation::Subtract},
            {"*", Operation::Multiply}, {"**", Operation::Power},
            {"/", Operation::Divide}, {"%", Operation::Modulo},
            {"//", Operation::IntDivide}, {"==", Operation::Equal},
            {"!=", Operation::NotEqual}, {">", Operation::Greater},
            {">=", Operation::GreaterEqual}, {"<", Operation::Less},
            {"<=", Operation::LessEqual}
        };

        const auto it = opMap.find(op);
        if (it == opMap.end()) {
            throw std::runtime_error("Unsupported operation: " + op.toStdString());
        }
        return it->second;
    }

    /**
     * @brief Вычисляет бинарную операцию над двумя строковыми значениями.
     *
     * Выполняет указанную операцию, такую как конкатенация или сравнение, между двумя строковыми значениями.
     * Если операция не поддерживается, выбрасывается исключение.
     *
     * @param l Левый операнд, представленный как `Value`, содержащий `QString`.
     * @param r Правый операнд, представленный как `Value`, содержащий `QString`.
     * @return `Value`, представляющий результат операции. Тип результата может варьироваться в зависимости от операции (например, конкатенация возвращает строку, сравнение возвращает логическое значение).
     * @throws std::runtime_error Если встречается неподдерживаемая операция.
     */
    [[nodiscard]] Value evalTwoStrings(const Value &l, const Value &r) const {
        const QString lv = std::get<QString>(l.data);
        const QString rv = std::get<QString>(r.data);

        switch (parseOperation(op)) {
            case Operation::Add: return Value(lv + rv);

            case Operation::Equal: return Value(lv == rv);
            case Operation::NotEqual: return Value(lv != rv);
            case Operation::LessEqual: return Value(lv.compare(rv) <= 0);
            case Operation::Less: return Value(lv.compare(rv) < 0);
            case Operation::GreaterEqual: return Value(lv.compare(rv) >= 0);
            case Operation::Greater: return Value(lv.compare(rv) > 0);
            default: throw std::runtime_error("Unsupported operation: " + op.toStdString() + "in evalTwoStrings");
        }
    }

    /**
     * @brief Вычисляет операцию между числовым и строковым значением.
     *
     * Этот метод выполняет операции между числовым значением (целым числом) и строковым значением
     * в соответствии с типом операции (`op`). В настоящее время поддерживается только оператор умножения (`*`),
     * который повторяет строку заданное количество раз.
     *
     * @param l Левый операнд, который может содержать целое число или строку.
     * @param r Правый операнд, который может содержать целое число или строку.
     * @return Объект Value, который содержит результат операции.
     *         Если `l` - целое число, а `r` - строка, результатом будет `r`, повторенная `l` раз.
     *         Если `l` - строка, а `r` - целое число, результатом будет `l`, повторенная `r` раз.
     * @throw std::runtime_error Если тип операции (`op`) не поддерживается.
     */
    [[nodiscard]] Value evalNumAndString(const Value &l, const Value &r) const {
        if (std::holds_alternative<int>(l.data)) {
            const int lv = std::get<int>(l.data);
            const QString rv = std::get<QString>(r.data);

            if (op == "*") return Value(rv.repeated(lv));
            throw std::runtime_error("Unsupported operation: " + op.toStdString());
        }
        const QString lv = std::get<QString>(l.data);
        const int rv = std::get<int>(r.data);
        if (op == "*") return Value(lv.repeated(rv));

        throw std::runtime_error("Unsupported operation: " + op.toStdString());
    }

    std::shared_ptr<ASTNode> left;
    QString op; // "+", "-", "=", "/", "%", "*", "**", "//", "=="
    std::shared_ptr<ASTNode> right;
};

/**
 * @class VarNode
 * @brief Представляет узел ссылки на переменную в абстрактном синтаксическом дереве (AST) интерпретатора.
 *
 * VarNode используется для представления переменных путем хранения их имен и предоставления механизмов
 * для определения их значений во время вычисления. Этот класс является финальным и не может быть далее унаследован.
 *
 * @details
 * Класс VarNode хранит имя переменной и предоставляет реализации методов
 * `eval` и `toString`. Во время вычисления он определяет значение переменной
 * путем поиска её в предоставленной среде выполнения. Также обеспечивает
 * представление имени переменной в виде строки.
 */
class VarNode final : public ASTNode {
public:
    explicit VarNode(QString name) : name(std::move(name)) {}

    QString name;
    [[nodiscard]] QString toString() const override { return name; }
    Value eval(Environment &env) const override { return env.get(name); }
};

class AssignNode final : public ASTNode {
public:
    AssignNode(QString varName, std::shared_ptr<ASTNode> valueExpr) :
    varName(std::move(varName)), valueExpr(std::move(valueExpr)) {}

    QString varName;
    std::shared_ptr<ASTNode> valueExpr;


    Value eval(Environment &env) const override {
        Value val = valueExpr->eval(env);
        env.set(varName, val);
        return val;
    }

    [[nodiscard]] QString toString() const override {
        return varName + " = " + valueExpr->toString();
    }
};

class IfNode final : public ASTNode {
public:
    IfNode(std::shared_ptr<ASTNode> condition,
        std::vector<std::shared_ptr<ASTNode>> body,
        std::vector<std::pair<std::shared_ptr<ASTNode>, std::vector<std::shared_ptr<ASTNode>>>> elifs,
        std::vector<std::shared_ptr<ASTNode>> elseBody)
    : condition(std::move(std::move(condition))), body(std::move(body)), elifs(std::move(elifs)), elseBody(std::move(elseBody)) {}

    Value eval(Environment &env) const override {
        if (condition->eval(env).toBool()) {
            Value lastValue;
            for (const auto& stmt : body) {
                lastValue = stmt->eval(env);
            }
            return lastValue;
        }

        for (const auto& elif: elifs) {
            if (elif.first->eval(env).toBool()) {
                Value lastValue;
                for (const auto& stmt : elif.second) {
                    lastValue = stmt->eval(env);
                }
                return lastValue;
            }
        }

        if (!elseBody.empty()) {
            Value lastValue;
            for (const auto& stmt : elseBody) {
                lastValue = stmt->eval(env);
            }
            return lastValue;
        }

        return Value();
    }

    [[nodiscard]] QString toString() const override {
        QString result = "if " + condition->toString() + ":\n";
        for (const auto& stmt : body) {
            result += "    " + stmt->toString() + "\n";
        }

        for (const auto& elif : elifs) {
            result += "elif " + elif.first->toString() + ":\n";
            for (const auto& stmt : elif.second) {
                result += "    " + stmt->toString() + "\n";
            }
        }

        if (!elseBody.empty()) {
            result += "else:\n";
            for (const auto& stmt : elseBody) {
                result += "    " + stmt->toString() + "\n";
            }
        }

        return result;
    }

private:
    std::shared_ptr<ASTNode> condition;
    std::vector<std::shared_ptr<ASTNode>> body;
    std::vector<std::pair<std::shared_ptr<ASTNode>, std::vector<std::shared_ptr<ASTNode>>>> elifs;
    std::vector<std::shared_ptr<ASTNode>> elseBody;
};

/**
 * @class Parser
 * @brief Выполняет разбор последовательности токенов в абстрактное синтаксическое дерево (AST).
 *
 * Parser организует разбор выражений на основании грамматики с учетом приоритетов операций.
 * Класс принимает поток токенов на вход и предоставляет соответствующий AST в качестве результата.
 * Используется в интерпретаторах, компиляторах и других системах анализа кода.
 *
 * @details
 * Основной метод `parse` возвращает корневой узел AST, представляющего собой анализируемое выражение.
 * Парсинг происходит в несколько этапов, начиная с самых низкоприоритетных операций до высокоприоритетных.
 * Класс включает методы, которые обеспечивают парсинг конкретных конструкций, таких как выражения со скобками,
 * операции унарного минуса, математические операции, сравнения и присваивания.
 * Если входные токены содержат синтаксические ошибки, генерируются исключения.
 */
class Parser {
public:
    /**
     * @brief Создает парсер с заданным списком токенов для анализа
     */
    explicit Parser(const QVector<Token> &tokens);

    /**
     * @brief Основной метод разбора, начинает анализ с выражений наивысшего приоритета
     * @return Корневой узел синтаксического дерева
     */
    std::shared_ptr<ASTNode> parse();

private:
    //Здесь методы разделены для анализа выражения согласно приоритету
    /**
     * @brief Разбирает операции присваивания (=)
     * @return Узел присваивания или выражение более высокого приоритета
     */
    std::shared_ptr<ASTNode> parseAssignment();

    /**
     * @brief Разбирает операции сравнения (==, !=, <, <=, >, >=)
     * @return Узел сравнения или выражение более высокого приоритета
     */
    std::shared_ptr<ASTNode> parseComparison();

    /**
     * @brief Разбирает операции сложения и вычитания (+, -)
     * @return Узел арифметической операции или выражение более высокого приоритета
     */
    std::shared_ptr<ASTNode> parseAdditionAndSubtraction();

    /**
     * @brief Разбирает операции умножения, деления и остатка (*, /, //, %)
     * @return Узел арифметической операции или выражение более высокого приоритета
     */
    std::shared_ptr<ASTNode> parseTerm();

    /**
     * @brief Разбирает унарный минус (-)
     * @return Узел унарного минуса или выражение более высокого приоритета
     */
    std::shared_ptr<ASTNode> parseUnaryMinus();

    /**
     * @brief Разбирает операцию возведения в степень (**)
     * @return Узел возведения в степень или первичное выражение
     */
    std::shared_ptr<ASTNode> parsePower();

    /**
     * @brief Разбирает первичные выражения (числа, строки, переменные, выражения в скобках)
     * @return Узел первичного выражения
     */
    std::shared_ptr<ASTNode> parsePrimary();

    /**
     * @brief Разбирает числовой токен
     * @return Узел числового значения
     */
    std::shared_ptr<ASTNode> parseNumberToken();

    /**
     * @brief Разбирает строковый токен
     * @return Узел строкового значения
     */
    std::shared_ptr<ASTNode> parseStringToken();

    /**
     * @brief Разбирает логический токен
     * @return Узел логического значения
     */
    std::shared_ptr<ASTNode> parseBoolToken();

    /**
     * @brief Разбирает токен идентификатора
     * @return Узел переменной
     */
    std::shared_ptr<ASTNode> parseIdentifierToken();

    /**
     * @brief Разбирает выражение в скобках
     * @return Узел выражения внутри скобок
     */
    std::shared_ptr<ASTNode> parseParenthesizedExpression();

    /**
     * @brief Возвращает текущий токен без продвижения
     * @return Текущий токен
     */
    [[nodiscard]] Token peek() const;

    /**
     * @brief Возвращает текущий токен и переходит к следующему
     * @return Текущий токен
     */
    Token advance();

    /**
     * @brief Выбрасывает ошибку о неожиданном токене
     * @param token Неожиданный токен
     */
    static void throwUnexpectedTokenError(const Token &token);

private:
    std::shared_ptr<ASTNode> parseIfStatement();
    std::vector<std::shared_ptr<ASTNode>> parseBlock();

    QVector<Token> tokens;
    int current = 0;
};
#endif // PARSER_H