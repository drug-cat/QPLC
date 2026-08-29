#pragma once

#include <string>
#include <vector>

enum class TokenType {
    IDENTIFIER,
    KEYWORD,
    INTEGER,
    FLOAT,
    TIME_LITERAL,
    OPERATOR,
    PUNCTUATION,
    INDENT,
    DEDENT,
    NEWLINE,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType t, std::string lex, int ln, int col)
        : type(t), lexeme(std::move(lex)), line(ln), column(col) {}
};

// Tokenize the entire source
std::vector<Token> tokenize(const std::string& source);

// Convert token type to string for display
std::string tokenTypeToString(TokenType type);