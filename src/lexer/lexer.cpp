#include "lexer/lexer.h"

#include <cctype>
#include <iostream>
#include <sstream>
#include <unordered_set>

using namespace std;

namespace {

// QPLC language keywords
const unordered_set<string> keywords = {
    "def", "if", "elif", "else", "while", "for", "in",
    "and", "or", "xor", "not", "return", "range", "True", "False",
    "break", "continue"
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

// Count indentation of a line (number of spaces / tab treated as 4 spaces)
int countIndent(const string& line) {
    int indent = 0;
    for (char c : line) {
        if (c == ' ') indent++;
        else if (c == '\t') indent += 4;  // tab = 4 spaces
        else break;
    }
    return indent;
}

// Strip leading whitespace from a line
string stripIndent(const string& line) {
    size_t pos = 0;
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
        pos++;
    }
    return line.substr(pos);
}

// Tokenize a single line (without indentation)
void tokenizeLine(const string& line, int lineNum, vector<Token>& tokens, int startCol) {
    size_t i = 0;
    const size_t n = line.size();
    int col = startCol;  // starting column (1-based)

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
            c == '<' || c == '>' || c == '%') {
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

namespace {

// Strip block comments /* */ from the entire source; newlines are preserved so line numbers stay correct
string stripBlockComments(const string& src) {
    string out = src;
    bool inComment = false;
    for (size_t i = 0; i < out.size(); ++i) {
        if (!inComment && i + 1 < out.size() && out[i] == '/' && out[i + 1] == '*') {
            inComment = true;
            out[i] = ' ';
            out[i + 1] = ' ';
            ++i;
            continue;
        }
        if (inComment) {
            if (i + 1 < out.size() && out[i] == '*' && out[i + 1] == '/') {
                out[i] = ' ';
                out[i + 1] = ' ';
                inComment = false;
                ++i;
            } else if (out[i] != '\n') {
                out[i] = ' ';
            }
        }
    }
    return out;
}

}  // namespace

vector<Token> tokenize(const string& source) {
    vector<Token> tokens;
    istringstream input(stripBlockComments(source));
    string line;
    int lineNum = 0;
    vector<int> indentStack = {0};  // current indentation level (starts at 0)

    while (getline(input, line)) {
        lineNum++;

        // Strip trailing \r (for Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // If the line contains only whitespace/tabs or is a comment, ignore it
        string trimmed = stripIndent(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        // Compute indentation
        int indent = countIndent(line);

        // Compare with the indentation stack
        if (indent > indentStack.back()) {
            // Increased indentation -> INDENT
            indentStack.push_back(indent);
            tokens.emplace_back(TokenType::INDENT, "", lineNum, 1);
        } else if (indent < indentStack.back()) {
            // Decreased indentation -> one or more DEDENT
            while (indent < indentStack.back()) {
                indentStack.pop_back();
                tokens.emplace_back(TokenType::DEDENT, "", lineNum, 1);
            }
            if (indent != indentStack.back()) {
                cerr << "Lexer error at line " << lineNum
                     << ": inconsistent dedent (no matching indentation level)\n";
                // Recovery: set the stack to the current level
                indentStack.push_back(indent);
            }
        }

        // Tokenize the line itself (without indentation)
        int startCol = indent + 1;  // column of the first non-space character
        string content = stripIndent(line);
        if (!content.empty()) {
            tokenizeLine(content, lineNum, tokens, startCol);
            // End of logical line -> NEWLINE
            tokens.emplace_back(TokenType::NEWLINE, "", lineNum, static_cast<int>(content.size()) + startCol);
        }
    }

    // At end of file: emit DEDENT for any still-open indentation levels
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