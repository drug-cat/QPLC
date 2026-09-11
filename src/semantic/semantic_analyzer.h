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
    const std::vector<SemanticError>& getErrors() const { return errors; }

private:
    const Config& config;
    std::vector<SemanticError> errors;
    std::vector<std::set<std::string>> localVarScopes;
    int loopDepth = 0;

    // User-defined function signatures for validating call sites
    std::set<std::string> declaredFunctions;
    std::unordered_map<std::string, int> functionParamCounts;

    // User-defined struct/enum names
    std::set<std::string> declaredStructs;
    std::set<std::string> declaredEnums;

    // Struct definitions (name -> field map)
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> structDefs;

    // Parameters of the function currently being analyzed — untyped (accept both bool and numeric)
    std::set<std::string> activeFunctionParams;
    bool isActiveParam(const std::string& name) const { return activeFunctionParams.count(name) > 0; }

    // Track struct type for local variables created by struct literals
    std::unordered_map<std::string, std::string> localVarStructTypes;

    void enterScope();
    void exitScope();
    void addLocalVar(const std::string& name);
    bool isLocalVar(const std::string& name) const;
    bool isConstant(const std::string& name) const;
    void reportUndefined(const Expr& expr, const std::string& name);
    void checkUserCall(const std::string& rawName, int argc, int line, int column);

    void checkStmt(const Stmt& stmt);
    void checkExpr(const Expr& expr);
    void checkReturnStmt(const ReturnStmt& stmt);

    bool isBoolExpr(const Expr& expr);
    bool isNumericOrTimeExpr(const Expr& expr);
    std::string getVarType(const std::string& name) const;
    int getArrayLength(const std::string& name) const;
    std::string getVarType(const std::string& name, bool& isArray) const;
    std::string getLocalStructType(const std::string& name) const;
    std::string getFieldType(const std::string& structName, const std::string& fieldName) const;
};