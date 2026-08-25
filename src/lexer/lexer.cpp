#include "lexer/lexer.h"

#include <cctype>
#include <iostream>
#include <sstream>
#include <unordered_set>

using namespace std;

namespace {

// کلمات کلیدی زبان QPLC
const unordered_set<string> keywords = {
    "def", "if", "elif", "else", "while", "for", "in",
    "and", "or", "not", "return", "range", "True", "False"
};

bool isKeyword(const string& word) {
    return keywords.find(word) != keywords.end();
}

bool isIdentifierStart(char c) {
    return isalpha(c) || c == '_';
}

bool isIdentifierChar(char c) {
    return isalnum(c) || c == '_';
}

// شمارش تورفتگی یک خط (تعداد فاصله‌ها / tab به‌عنوان ۴ فاصله)
int countIndent(const string& line) {
    int indent = 0;
    for (char c : line) {
        if (c == ' ') indent++;
        else if (c == '\t') indent += 4;  // tab = 4 spaces
        else break;
    }
    return indent;
}

// حذف فاصله‌های ابتدای خط
string stripIndent(const string& line) {
    size_t pos = 0;
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
        pos++;
    }
    return line.substr(pos);
}

// توکنایز کردن یک خط (بدون تورفتگی)
void tokenizeLine(const string& line, int lineNum, vector<Token>& tokens, int startCol) {
    size_t i = 0;
    const size_t n = line.size();
    int col = startCol;  // ستون شروع (۱-based)

    auto addToken = [&](TokenType type, const string& lexeme, int tokenCol) {
        tokens.emplace_back(type, lexeme, lineNum, tokenCol);
    };

    while (i < n) {
        char c = line[i];

        // Skip spaces
        if (isspace(c)) {
            i++;
            col++;
            continue;
        }

        // Comment (rest of line)
        if (c == '#') {
            break;  // ignore rest of line
        }

        // Time literal (T#...) - MUST be checked BEFORE identifier
        if (c == 'T' && i + 1 < n && line[i + 1] == '#') {
            size_t start = i;
            int startCol = col;
            i += 2;  // skip T#
            col += 2;
            while (i < n && (isalnum(line[i]) || line[i] == '_')) {
                i++;
                col++;
            }
            string timeStr = line.substr(start, i - start);
            addToken(TokenType::TIME_LITERAL, timeStr, startCol);
            continue;
        }

        // Identifier or keyword
        if (isIdentifierStart(c)) {
            size_t start = i;
            int startCol = col;
            while (i < n && isIdentifierChar(line[i])) {
                i++;
                col++;
            }
            string word = line.substr(start, i - start);
            if (isKeyword(word)) {
                addToken(TokenType::KEYWORD, word, startCol);
            } else {
                addToken(TokenType::IDENTIFIER, word, startCol);
            }
            continue;
        }

        // Number (integer or float)
        if (isdigit(c)) {
            size_t start = i;
            int startCol = col;
            bool isFloat = false;
            while (i < n && (isdigit(line[i]) || line[i] == '.')) {
                if (line[i] == '.') isFloat = true;
                i++;
                col++;
            }
            string numStr = line.substr(start, i - start);
            if (isFloat) {
                addToken(TokenType::FLOAT, numStr, startCol);
            } else {
                addToken(TokenType::INTEGER, numStr, startCol);
            }
            continue;
        }

        // Multi-character operators
        if (i + 1 < n) {
            string two = line.substr(i, 2);
            if (two == "==" || two == "!=" || two == "<=" || two == ">=") {
                addToken(TokenType::OPERATOR, two, col);
                i += 2;
                col += 2;
                continue;
            }
        }

        // Single-character operators and punctuation
        if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '<' || c == '>') {
            addToken(TokenType::OPERATOR, string(1, c), col);
            i++;
            col++;
            continue;
        }

        if (c == '(' || c == ')' || c == ':' || c == '[' || c == ']' || c == '.' || c == ',') {
            addToken(TokenType::PUNCTUATION, string(1, c), col);
            i++;
            col++;
            continue;
        }

        // Unknown character
        cerr << "Lexer error at line " << lineNum << ", col " << col
             << ": unexpected character '" << c << "'\n";
        i++;
        col++;
    }
}

}  // namespace

vector<Token> tokenize(const string& source) {
    vector<Token> tokens;
    istringstream input(source);
    string line;
    int lineNum = 0;
    vector<int> indentStack = {0};  // سطح تورفتگی فعلی (شروع با ۰)

    while (getline(input, line)) {
        lineNum++;

        // حذف \r در انتهای خط (برای ویندوز)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // اگر خط فقط شامل فاصله/تب باشد یا کامنت باشد، آن را نادیده می‌گیریم
        string trimmed = stripIndent(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        // محاسبه تورفتگی
        int indent = countIndent(line);

        // مقایسه با استک تورفتگی
        if (indent > indentStack.back()) {
            // تورفتگی بیشتر -> INDENT
            indentStack.push_back(indent);
            tokens.emplace_back(TokenType::INDENT, "", lineNum, 1);
        } else if (indent < indentStack.back()) {
            // تورفتگی کمتر -> چند DEDENT
            while (indent < indentStack.back()) {
                indentStack.pop_back();
                tokens.emplace_back(TokenType::DEDENT, "", lineNum, 1);
            }
            if (indent != indentStack.back()) {
                cerr << "Lexer error at line " << lineNum
                     << ": inconsistent dedent (no matching indentation level)\n";
                // بازیابی: تنظیم استک به سطح فعلی
                indentStack.push_back(indent);
            }
        }

        // توکنایز کردن خود خط (بدون تورفتگی)
        int startCol = indent + 1;  // ستون اولین کاراکتر غیرفضا
        string content = stripIndent(line);
        if (!content.empty()) {
            tokenizeLine(content, lineNum, tokens, startCol);
            // خط منطقی تمام شده -> NEWLINE
            tokens.emplace_back(TokenType::NEWLINE, "", lineNum, static_cast<int>(content.size()) + startCol);
        }
    }

    // در پایان فایل: اگر خطوط باز داریم، DEDENT صادر کن
    while (indentStack.size() > 1) {
        indentStack.pop_back();
        tokens.emplace_back(TokenType::DEDENT, "", lineNum, 1);
    }

    // EOF
    tokens.emplace_back(TokenType::END_OF_FILE, "", lineNum, 1);
    return tokens;
}

string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER:   return "IDENTIFIER";
        case TokenType::KEYWORD:      return "KEYWORD";
        case TokenType::INTEGER:      return "INTEGER";
        case TokenType::FLOAT:        return "FLOAT";
        case TokenType::TIME_LITERAL: return "TIME_LITERAL";
        case TokenType::OPERATOR:     return "OPERATOR";
        case TokenType::PUNCTUATION:  return "PUNCTUATION";
        case TokenType::INDENT:       return "INDENT";
        case TokenType::DEDENT:       return "DEDENT";
        case TokenType::NEWLINE:      return "NEWLINE";
        case TokenType::END_OF_FILE:  return "END_OF_FILE";
    }
    return "UNKNOWN";
}