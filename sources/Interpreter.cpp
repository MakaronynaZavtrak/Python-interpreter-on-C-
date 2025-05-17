#ifdef _WIN32
#include <windows.h>
#endif
#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
    struct ConsoleModeGuard {
        HANDLE hIn;
        static DWORD s_systemMode;
        DWORD previousMode;
        ConsoleModeGuard()
          : hIn(GetStdHandle(STD_INPUT_HANDLE))
        {
            DWORD current;
            // Сохраняем текущий режим
            GetConsoleMode(hIn, &current);

            // Если ещё не инициализировали статик — сохраняем
            if (s_systemMode == DWORD(-1)) {
                s_systemMode = current;
            }

            previousMode = current;

            // Настраиваем свой режим
            DWORD mode = s_systemMode;
            mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
            mode |= ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
            SetConsoleMode(hIn, mode);
        }
        ~ConsoleModeGuard() {
            // Восстанавливаем оригинальный режим
            SetConsoleMode(hIn, s_systemMode);
        }
    };
    DWORD ConsoleModeGuard::s_systemMode = DWORD(-1);

struct PromptLine {
    COORD absPos;
    std::string prompt;  // ">>> " или "... "
    std::string buffer;  // введённый текст
};
#endif

COORD getCursorPosition() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.dwCursorPosition;
}


void Interpreter::run(int argc, char* argv[]) {
#ifdef _WIN32
    // Настройка консоли
    ConsoleModeGuard consoleGuard;
    HANDLE hIn = consoleGuard.hIn;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT oldWindow;
    {   // первая инициализация oldWindow
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        oldWindow = csbi.srWindow;
    }
#endif

    std::cout
      << "Hello and welcome to my minimal Python interpreter!\n"
         "Made by Semenov Oleg, with care from MathMech. Let's code!\n";
#ifdef _WIN32
    // История
    std::vector<std::string> inputHistory;
    int inputHistoryIndex = -1;
    std::vector<PromptLine> history;
    int blockStartIndex = -1;
    int currentLine = -1;
    bool inBlock = false;
    bool editingHistory = false;
    std::string block;
    bool firstPrompt = true;

    auto gotoPrompt = [&](const PromptLine &pl) {
        SetConsoleCursorPosition(hOut, pl.absPos);
    };


    auto newPrompt = [&](const std::string& promptText, bool addNewline = true) {
        if (addNewline && !firstPrompt) {
            std::cout << "\r\n";
        }
        firstPrompt = false;

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        DWORD written;
        WriteConsoleA(hOut, promptText.c_str(), (DWORD)promptText.size(), &written, nullptr);
        SetConsoleTextAttribute(hOut, csbi.wAttributes);

        CONSOLE_SCREEN_BUFFER_INFO csbi2;
        GetConsoleScreenBufferInfo(hOut, &csbi2);
        COORD startPos = { SHORT(csbi2.dwCursorPosition.X - promptText.size()), csbi2.dwCursorPosition.Y };
        history.push_back({ startPos, promptText, "" });
        currentLine = (int)history.size() - 1;

        bool wasIn = inBlock;
        inBlock = (promptText == "... ");
        if (inBlock && !wasIn) blockStartIndex = currentLine;
    };


    newPrompt(">>> ");
    Environment env;
    Lexer lexer;

    INPUT_RECORD rec;
    DWORD read;


auto renderHistoryBlock = [&](const std::string& textBlock) {
        DWORD w;
        while (history.size() > 1 && history.back().prompt == "... ") {
            auto &e = history.back();
            gotoPrompt(e);
            std::string blanks(e.prompt.size() + e.buffer.size(), ' ');
            WriteConsoleA(hOut, blanks.c_str(), (DWORD)blanks.size(), &w, nullptr);
            history.pop_back();
        }

        std::vector<std::string> lines;
        std::istringstream iss(textBlock);
        std::string ln;
        while (std::getline(iss, ln)) lines.push_back(ln);
        if (lines.empty()) return;

        auto &prim = history.back();
        prim.buffer = lines[0];
        gotoPrompt(prim);

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        WriteConsoleA(hOut, prim.prompt.c_str(), (DWORD)prim.prompt.size(), &w, nullptr);
        SetConsoleTextAttribute(hOut, csbi.wAttributes);
        WriteConsoleA(hOut, prim.buffer.c_str(), (DWORD)prim.buffer.size(), &w, nullptr);

        for (size_t i = 1; i < lines.size(); ++i) {
            std::cout << "\r\n";
            CONSOLE_SCREEN_BUFFER_INFO csbi2;
            GetConsoleScreenBufferInfo(hOut, &csbi2);
            PromptLine cont{ {csbi2.dwCursorPosition.X, csbi2.dwCursorPosition.Y}, "... ", lines[i] };
            history.push_back(cont);

            SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            WriteConsoleA(hOut, cont.prompt.c_str(), (DWORD)cont.prompt.size(), &w, nullptr);
            SetConsoleTextAttribute(hOut, csbi2.wAttributes);
            WriteConsoleA(hOut, cont.buffer.c_str(), (DWORD)cont.buffer.size(), &w, nullptr);
        }

        currentLine = (int)history.size() - 1;
        if (lines.size() > 1) {
            inBlock = true;
            blockStartIndex = currentLine - (int)lines.size() + 1;
            block = textBlock;
        } else {
            inBlock = false;
            blockStartIndex = -1;
            block.clear();
        }
    };


    auto clearCurrentHistoryBlock = [&]() {
        DWORD written;
        while (history.size() > 1 && history.back().prompt == "... ") {
            auto &e = history.back();
            FillConsoleOutputCharacterA(hOut, ' ', e.prompt.size() + e.buffer.size(), e.absPos, &written);
            history.pop_back();
        }
        currentLine = (int)history.size() - 1;
        if (!history.empty()) {
            auto &prim = history.back();
            if (!prim.buffer.empty()) {
                COORD bufStart{ SHORT(prim.absPos.X + prim.prompt.size()), prim.absPos.Y };
                FillConsoleOutputCharacterA(hOut, ' ', prim.buffer.size(), bufStart, &written);
                prim.buffer.clear();
            }
            SetConsoleCursorPosition(hOut, { SHORT(prim.absPos.X + prim.prompt.size()), prim.absPos.Y });
        }
    };



    while (true) {
        // Проверяем автоскролл/прокрутку
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        SHORT deltaY = csbi.srWindow.Top - oldWindow.Top;
        if (deltaY != 0) {
            for (auto &pl : history) pl.absPos.Y -= deltaY;
            oldWindow = csbi.srWindow;
        }

        ReadConsoleInput(hIn, &rec, 1, &read);

        // === Handle real window-size changes ===
        if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            // Capture new window rectangle
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            SMALL_RECT newWindow = csbi.srWindow;
            // Compute vertical scroll delta
            SHORT deltaY = newWindow.Top - oldWindow.Top;
            if (deltaY != 0) {
                // Adjust all stored prompt positions
                for (auto &pl : history) {
                    pl.absPos.Y -= deltaY;
                }
                // Update oldWindow for future events
                oldWindow = newWindow;
            }
            continue;
        }

        // === Handle mouse wheel scroll events ===
        if (rec.EventType == MOUSE_EVENT &&
            (rec.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED))
        {
            // Получаем signed delta колёсика (HIWORD)
            SHORT wheelDelta = (SHORT)(rec.Event.MouseEvent.dwButtonState >> 16);
            // Количество строк: обычно wheelDelta = ±120
            int lines = wheelDelta / WHEEL_DELTA;
            // Сдвигаем историю на столько же строк
            for (auto &pl : history) {
                pl.absPos.Y -= lines;
            }
            continue;
        }

        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown)
            continue;

        auto& pl = history[currentLine];
        WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
        char ch = rec.Event.KeyEvent.uChar.AsciiChar;

        // ENTER
if (vk == VK_RETURN) {
    std::string line = pl.buffer;

    if (inBlock && blockStartIndex < 0) {
        inBlock = false;
        editingHistory = false;
        block.clear();
        newPrompt(">>> ");
        continue;
    }

    // 0) Пустой Enter на главном prompt-е игнорируем:
    if (!inBlock && line.empty()) {
        newPrompt(">>> ");
        continue;
    }

    // 1) Команда выхода
    if (!inBlock && (line == "exit" || line == "quit" || line == "q")) {
        break;
    }

    // 2) Start block (line.back() == ':')
    if (!inBlock && !line.empty() && line.back() == ':') {
        block = line;
        newPrompt("... ");
        editingHistory = false;
        continue;
    }

    // 3) Continue block
    if (inBlock && !line.empty()) {
        if (editingHistory) {
            // перезаписываем историю: не наслаиваем старые строки
            // block = line;
            std::ostringstream oss;
            for (int i = blockStartIndex; i <= currentLine; ++i) {
                if (i > blockStartIndex) oss << "\n";
                    oss << history[i].buffer;
            }
            block = oss.str();
            editingHistory = false;
        } else {
            // обычный ввод — добавляем новую строку
            block += "\n" + line;
        }
        newPrompt("... ");
        continue;
    }

    // 4) End block (empty line inBlock)
    if (inBlock && line.empty()) {
        if (blockStartIndex < 0) {
            // Просто сбрасываем
            inBlock = false;
            editingHistory = false;
            newPrompt(">>> ");
            continue;
        }
        // сохраняем full block
        if (!block.empty()) {
            if (inputHistory.empty() || inputHistory.back() != block) {
                inputHistory.push_back(block);
            }
            inputHistoryIndex = -1;
        }
        editingHistory = false;
        // выполняем блок
        try {
            auto tokens = lexer.tokenize(QString::fromStdString(block));
            auto ast = Parser(tokens).parse();
            ast->eval(env);
        } catch (const std::runtime_error& e) {
            std::cout << "\nError: " << e.what();
        }
        // сбрасываем состояние блока
        inBlock = false;
        blockStartIndex = -1;
        block.clear();
        newPrompt(">>> ");
        continue;
    }

    // 5) Однострочная команда
    if (!inBlock && !line.empty()) {
        if (inputHistory.empty() || inputHistory.back() != line) {
            inputHistory.push_back(line);
        }
        inputHistoryIndex = -1;
        editingHistory = false;
        // eval
        try {
            auto tokens = lexer.tokenize(QString::fromStdString(line));
            auto ast = Parser(tokens).parse();
            auto result = ast->eval(env);
            if (!result.toString().isEmpty() &&
                !dynamic_cast<AssignNode*>(ast.get()))
            {
                std::cout << "\n" << result.toString().toStdString();
            }
        } catch (const std::runtime_error& e) {
            std::cout << "\nError: " << e.what() << "\n";
        }

    }
    // 6) Новый prompt
    newPrompt(">>> ");
    continue;
}


        // TAB
        if (vk == VK_TAB) {
            pl.buffer += "    ";
            DWORD w;
            WriteConsoleA(hOut, "    ", 4, &w, nullptr);
            continue;
        }

        // --- BACKSPACE ---

if (vk == VK_BACK) {
    auto &pl = history[currentLine];

    // 0) Защита от удаления первого prompt-а:
    if (pl.prompt == ">>> " && pl.buffer.empty()) {
        continue;
    }

    // 1) Если в буфере есть символ — обычное удаление справа налево:
    if (!pl.buffer.empty()) {
        pl.buffer.pop_back();
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        COORD p = csbi.dwCursorPosition;
        p.X = SHORT(p.X - 1);
        SetConsoleCursorPosition(hOut, p);
        DWORD written;
        WriteConsoleA(hOut, " ", 1, &written, nullptr);
        SetConsoleCursorPosition(hOut, p);
        continue;
    }

    // 2) Пустой continuation-prompt — убираем его целиком
    if (pl.prompt == "... ") {
        DWORD written;
        FillConsoleOutputCharacterA(
            hOut,
            ' ',
            pl.prompt.size() + pl.buffer.size(),
            pl.absPos,
            &written
        );

        history.pop_back();
        currentLine = (int)history.size() - 1;
        auto &prev = history[currentLine];
        COORD end = {
            SHORT(prev.absPos.X + prev.prompt.size() + prev.buffer.size()),
            prev.absPos.Y
            };
        SetConsoleCursorPosition(hOut, end);

        // 2.4) Завершаем блок, если это был ">>> "
        if (prev.prompt == ">>> ") {
            inBlock = false;
            blockStartIndex = -1;
        }

        continue;
    }
}

        // UP
        if (vk == VK_UP) {
            if (inputHistoryIndex + 1 < (int)inputHistory.size()) {
                inputHistoryIndex++;
                clearCurrentHistoryBlock();
                renderHistoryBlock(inputHistory[inputHistory.size() - 1 - inputHistoryIndex]);
                editingHistory = true;
            }
            continue;
        }

        // DOWN
        if (vk == VK_DOWN) {
            if (inputHistory.empty() || inputHistoryIndex == -1) {
                editingHistory   = false;
                inBlock          = false;
                blockStartIndex  = -1;
                currentLine = static_cast<int>(history.size()) - 1;
                // курсор остаётся сразу после единственного ">>> "
                const auto &prim = history.back();
                COORD after = {
                    SHORT(prim.absPos.X + prim.prompt.size()),
                    prim.absPos.Y
                };
                SetConsoleCursorPosition(hOut, after);
                continue;
            }
            if (inputHistoryIndex > 0) {
                inputHistoryIndex--;
                clearCurrentHistoryBlock(); // обязательно очищаем текущий ввод
                renderHistoryBlock(inputHistory[inputHistory.size() - 1 - inputHistoryIndex]);
                editingHistory = true;
            } else {
                inputHistoryIndex = -1;
                clearCurrentHistoryBlock(); // очистить текущий блок
                editingHistory = false;
                inBlock = false;
                blockStartIndex = -1;
                const auto &prim = history.back();
                COORD afterPrompt = {
                    SHORT(prim.absPos.X + prim.prompt.size()),
                    prim.absPos.Y
                };
                SetConsoleCursorPosition(hOut, afterPrompt);
            }
            continue;
        }


        // Printable chars
        if (ch >= 32 && ch <= 126) {
            pl.buffer.push_back(ch);
            DWORD w;
            WriteConsoleA(hOut, &ch, 1, &w, nullptr);
        }
    }
#else
    // fallback
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "exit" || line == "quit" || line == "q") break;
        try {
            auto tokens = lexer.tokenize(QString::fromStdString(line));
            auto ast = Parser(tokens).parse();
            auto result = ast->eval(env);
            if (!result.toString().isEmpty() &&
                !dynamic_cast<AssignNode*>(ast.get()))
            {
                std::cout << result.toString().toStdString() << "\n";
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
#endif
}