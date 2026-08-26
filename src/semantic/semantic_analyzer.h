#pragma once

#include "ast/ast.h"
#include "config/config_parser.h"
#include <vector>
#include <string>
#include <set>
#include <unordered_map>

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
    int loopDepth = 0;

    // امضای توابع کاربر برای اعتبارسنجی فراخوانی‌ها
    std::set<std::string> declaredFunctions;
    std::unordered_map<std::string, int> functionParamCounts;

    // پارامترهای تابعِ در حال بررسی — بی‌نوع هستند (هم بولی هم عددی می‌پذیرند)
    std::set<std::string> activeFunctionParams;
    bool isActiveParam(const std::string& name) const { return activeFunctionParams.count(name) > 0; }

    void enterScope();
    void exitScope();
    void addLocalVar(const std::string& name);
    bool isLocalVar(const std::string& name) const;
    bool isConstant(const std::string& name) const;
    void reportUndefined(const Expr& expr, const std::string& name);
    void checkUserCall(const std::string& rawName, int argc, int line, int column);

    void checkStmt(const Stmt& stmt);
    void checkExpr(const Expr& expr);

    bool isBoolExpr(const Expr& expr);
    bool isNumericOrTimeExpr(const Expr& expr);
    std::string getVarType(const std::string& name) const;
    int getArrayLength(const std::string& name) const;
    std::string getVarType(const std::string& name, bool& isArray) const;
};