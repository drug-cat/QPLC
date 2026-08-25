#pragma once

#include <vector>
#include <memory>
#include <string>

#include "lexer/lexer.h"
#include "ast/ast.h"

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    std::unique_ptr<Program> parseProgram();

private:
    const std::vector<Token>& tokens;
    size_t pos = 0;

    const Token& current() const;
    const Token& peek(size_t offset = 1) const;
    bool check(TokenType type, const std::string& lexeme = "") const;
    const Token& advance();
    const Token& expect(TokenType type, const std::string& lexeme = "");
    void skipNewlines();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<FunctionDef> parseFunctionDef();
    std::vector<StmtPtr> parseBlock();
    std::vector<StmtPtr> parseStatements();
    StmtPtr parseStatement();
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<ForStmt> parseForStmt();
    StmtPtr parseAssignment();          // handle both simple and index assignment

    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseNot();
    ExprPtr parseComparison();
    ExprPtr parseAdditive();
    ExprPtr parseMultiplicative();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();
};