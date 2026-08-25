#pragma once

#include "ast/ast.h"
#include "config/config_parser.h"
#include <vector>
#include <string>
#include <set>

struct SemanticError {
    int line;
    int column;
    std::string message;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer(const Config& cfg);
    std::vector<SemanticError> analyze(const Program& program);

private:
    const Config& config;
    std::vector<SemanticError> errors;
    std::vector<std::set<std::string>> localVarScopes;

    void enterScope();
    void exitScope();
    void addLocalVar(const std::string& name);
    bool isLocalVar(const std::string& name) const;

    void checkStmt(const Stmt& stmt);
    void checkExpr(const Expr& expr);

    bool isBoolExpr(const Expr& expr);
    bool isNumericOrTimeExpr(const Expr& expr);
    std::string getVarType(const std::string& name) const;
    int getArrayLength(const std::string& name) const;
    std::string getVarType(const std::string& name, bool& isArray) const;
};