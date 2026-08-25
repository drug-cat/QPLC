#include "semantic/semantic_analyzer.h"

#include <unordered_set>

using namespace std;

namespace {
    const unordered_set<string> timerFunctions = {
        "on_delay", "off_delay", "pulse"
    };
    const unordered_set<string> counterFunctions = {
        "count_up", "count_down", "count_updown"
    };
}

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
SemanticAnalyzer::SemanticAnalyzer(const Config& cfg)
    : config(cfg)
{
    localVarScopes.push_back({});   // global scope
}

//------------------------------------------------------------------------------
// Scope Management
//------------------------------------------------------------------------------
void SemanticAnalyzer::enterScope() {
    localVarScopes.push_back({});
}

void SemanticAnalyzer::exitScope() {
    if (!localVarScopes.empty()) {
        localVarScopes.pop_back();
    }
}

void SemanticAnalyzer::addLocalVar(const string& name) {
    if (!localVarScopes.empty()) {
        localVarScopes.back().insert(name);
    }
}

bool SemanticAnalyzer::isLocalVar(const string& name) const {
    for (const auto& scope : localVarScopes) {
        if (scope.find(name) != scope.end()) {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------
string SemanticAnalyzer::getVarType(const string& name) const {
    auto it = config.io.find(name);
    if (it != config.io.end()) {
        return it->second.type;
    }
    return "";
}

int SemanticAnalyzer::getArrayLength(const string& name) const {
    auto it = config.io.find(name);
    if (it != config.io.end()) {
        return it->second.arrayLength;
    }
    return 1;
}

string SemanticAnalyzer::getVarType(const string& name, bool& isArray) const {
    auto it = config.io.find(name);
    if (it != config.io.end()) {
        isArray = (it->second.arrayLength > 1);
        return it->second.type;
    }
    isArray = false;
    return "";
}

//------------------------------------------------------------------------------
// Expression Type Checks
//------------------------------------------------------------------------------
bool SemanticAnalyzer::isBoolExpr(const Expr& expr) {
    if (auto var = dynamic_cast<const VarExpr*>(&expr)) {
        if (isLocalVar(var->name)) {
            // loop variable is numeric, so not boolean
            return false;
        }
        string t = getVarType(var->name);
        if (t.empty()) {
            errors.push_back({var->line, var->column,
                "Variable '" + var->name + "' is not defined in conf.qplc"});
            return false;
        }
        return t == "BOOL";
    }
    if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        bool isArray = false;
        string t = getVarType(idx->name, isArray);
        if (t.empty()) {
            errors.push_back({idx->line, idx->column,
                "Variable '" + idx->name + "' is not defined in conf.qplc"});
            return false;
        }
        if (!isArray) {
            errors.push_back({idx->line, idx->column,
                "Variable '" + idx->name + "' is not an array"});
            return false;
        }
        // check index bounds if constant
        if (auto numIdx = dynamic_cast<const NumberExpr*>(idx->index.get())) {
            int index = stoi(numIdx->value);
            int len = getArrayLength(idx->name);
            if (index < 0 || index >= len) {
                errors.push_back({idx->line, idx->column,
                    "Array index out of bounds for '" + idx->name + "' (index " + to_string(index) + ", length " + to_string(len) + ")"});
            }
        }
        return t == "BOOL";
    }
    if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        // timer and counter functions return BOOL
        if (timerFunctions.find(call->funcName) != timerFunctions.end() ||
            counterFunctions.find(call->funcName) != counterFunctions.end()) {
            return true;
        }
        // unknown function: let checkExpr report error
        return false;
    }
    if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        // For now, assume attributes (like timer1.Q) are boolean
        // TODO: proper type resolution
        return true;
    }
    if (dynamic_cast<const BoolExpr*>(&expr)) {
        return true;
    }
    if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (bin->op == "and" || bin->op == "or") {
            return isBoolExpr(*bin->left) && isBoolExpr(*bin->right);
        }
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" ||
            bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
            // comparison is always BOOL
            return true;
        }
        return false;
    }
    if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (un->op == "not") {
            return isBoolExpr(*un->operand);
        }
        return false;
    }
    return false;
}

bool SemanticAnalyzer::isNumericOrTimeExpr(const Expr& expr) {
    if (dynamic_cast<const NumberExpr*>(&expr)) {
        return true;
    }
    if (dynamic_cast<const TimeExpr*>(&expr)) {
        return true;
    }
    if (auto var = dynamic_cast<const VarExpr*>(&expr)) {
        if (isLocalVar(var->name)) {
            // loop variable is numeric
            return true;
        }
        string t = getVarType(var->name);
        if (t.empty()) {
            errors.push_back({var->line, var->column,
                "Variable '" + var->name + "' is not defined in conf.qplc"});
            return false;
        }
        return t != "BOOL";
    }
    if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        bool isArray = false;
        string t = getVarType(idx->name, isArray);
        if (t.empty()) {
            errors.push_back({idx->line, idx->column,
                "Variable '" + idx->name + "' is not defined in conf.qplc"});
            return false;
        }
        if (!isArray) {
            errors.push_back({idx->line, idx->column,
                "Variable '" + idx->name + "' is not an array"});
            return false;
        }
        if (auto numIdx = dynamic_cast<const NumberExpr*>(idx->index.get())) {
            int index = stoi(numIdx->value);
            int len = getArrayLength(idx->name);
            if (index < 0 || index >= len) {
                errors.push_back({idx->line, idx->column,
                    "Array index out of bounds for '" + idx->name + "' (index " + to_string(index) + ", length " + to_string(len) + ")"});
            }
        }
        return t != "BOOL";
    }
    if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        // Timers/counters are boolean, not numeric
        return false;
    }
    if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (bin->op == "+" || bin->op == "-" || bin->op == "*" || bin->op == "/") {
            return isNumericOrTimeExpr(*bin->left) && isNumericOrTimeExpr(*bin->right);
        }
        return false;
    }
    return false;
}

//------------------------------------------------------------------------------
// Statement Checking
//------------------------------------------------------------------------------
void SemanticAnalyzer::checkStmt(const Stmt& stmt) {
    if (auto assign = dynamic_cast<const AssignmentStmt*>(&stmt)) {
        auto it = config.io.find(assign->name);
        if (it == config.io.end()) {
            errors.push_back({assign->line, assign->column,
                "Variable '" + assign->name + "' is not defined in conf.qplc"});
        } else {
            string varType = it->second.type;
            if (varType == "BOOL") {
                if (!isBoolExpr(*assign->expr)) {
                    errors.push_back({assign->line, assign->column,
                        "Type mismatch: variable '" + assign->name +
                        "' is BOOL but expression is not boolean"});
                }
            } else {
                if (!isNumericOrTimeExpr(*assign->expr)) {
                    errors.push_back({assign->line, assign->column,
                        "Type mismatch: variable '" + assign->name +
                        "' is " + varType + " but expression is not numeric/time"});
                }
            }
        }
    }
    else if (auto idxAssign = dynamic_cast<const IndexAssignmentStmt*>(&stmt)) {
        bool isArray = false;
        string varType = getVarType(idxAssign->name, isArray);
        if (varType.empty()) {
            errors.push_back({idxAssign->line, idxAssign->column,
                "Variable '" + idxAssign->name + "' is not defined in conf.qplc"});
        } else if (!isArray) {
            errors.push_back({idxAssign->line, idxAssign->column,
                "Variable '" + idxAssign->name + "' is not an array"});
        } else {
            // check index bounds if constant
            if (auto numIdx = dynamic_cast<const NumberExpr*>(idxAssign->index.get())) {
                int index = stoi(numIdx->value);
                int len = getArrayLength(idxAssign->name);
                if (index < 0 || index >= len) {
                    errors.push_back({idxAssign->line, idxAssign->column,
                        "Array index out of bounds for '" + idxAssign->name + "' (index " + to_string(index) + ", length " + to_string(len) + ")"});
                }
            }
            if (varType == "BOOL") {
                if (!isBoolExpr(*idxAssign->expr)) {
                    errors.push_back({idxAssign->line, idxAssign->column,
                        "Type mismatch: array element '" + idxAssign->name +
                        "' is BOOL but expression is not boolean"});
                }
            } else {
                if (!isNumericOrTimeExpr(*idxAssign->expr)) {
                    errors.push_back({idxAssign->line, idxAssign->column,
                        "Type mismatch: array element '" + idxAssign->name +
                        "' is " + varType + " but expression is not numeric/time"});
                }
            }
        }
    }
    else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        if (!isBoolExpr(*ifStmt->cond)) {
            errors.push_back({ifStmt->cond->line, ifStmt->cond->column,
                "Condition must be boolean"});
        }
        for (const auto& s : ifStmt->thenBlock) {
            checkStmt(*s);
        }
        for (const auto& branch : ifStmt->elifBranches) {
            if (!isBoolExpr(*branch.first)) {
                errors.push_back({branch.first->line, branch.first->column,
                    "Condition must be boolean"});
            }
            for (const auto& s : branch.second) {
                checkStmt(*s);
            }
        }
        for (const auto& s : ifStmt->elseBlock) {
            checkStmt(*s);
        }
    }
    else if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        if (forStmt->end <= 0) {
            errors.push_back({forStmt->line, forStmt->column,
                "For loop range must be a positive integer"});
        }
        enterScope();
        addLocalVar(forStmt->varName);
        for (const auto& innerStmt : forStmt->body) {
            checkStmt(*innerStmt);
        }
        exitScope();
    }
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        if (!isBoolExpr(*whileStmt->cond)) {
            errors.push_back({whileStmt->cond->line, whileStmt->cond->column,
                "Condition must be boolean"});
        }
        for (const auto& s : whileStmt->body) {
            checkStmt(*s);
        }
    }
}

//------------------------------------------------------------------------------
// Expression Checking
//------------------------------------------------------------------------------
void SemanticAnalyzer::checkExpr(const Expr& expr) {
    if (auto var = dynamic_cast<const VarExpr*>(&expr)) {
        if (config.io.find(var->name) == config.io.end() && !isLocalVar(var->name)) {
            errors.push_back({var->line, var->column,
                "Variable '" + var->name + "' is not defined in conf.qplc"});
        }
    }
    else if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        if (config.io.find(idx->name) == config.io.end()) {
            errors.push_back({idx->line, idx->column,
                "Variable '" + idx->name + "' is not defined in conf.qplc"});
        } else {
            if (auto numIdx = dynamic_cast<const NumberExpr*>(idx->index.get())) {
                int index = stoi(numIdx->value);
                int len = getArrayLength(idx->name);
                if (index < 0 || index >= len) {
                    errors.push_back({idx->line, idx->column,
                        "Array index out of bounds for '" + idx->name + "' (index " + to_string(index) + ", length " + to_string(len) + ")"});
                }
            }
        }
        if (idx->index) checkExpr(*idx->index);
    }
    else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        const string& name = call->funcName;
        if (timerFunctions.find(name) != timerFunctions.end()) {
            // timer functions: (BOOL, TIME)
            if (call->args.size() != 2) {
                errors.push_back({call->line, call->column,
                    "Function '" + name + "' expects 2 arguments (input, time)"});
            } else {
                if (!isBoolExpr(*call->args[0])) {
                    errors.push_back({call->args[0]->line, call->args[0]->column,
                        "First argument of '" + name + "' must be boolean"});
                }
                if (!isNumericOrTimeExpr(*call->args[1])) {
                    errors.push_back({call->args[1]->line, call->args[1]->column,
                        "Second argument of '" + name + "' must be TIME"});
                }
            }
        }
        else if (counterFunctions.find(name) != counterFunctions.end()) {
            if (name == "count_up") {
                if (call->args.size() != 3) {
                    errors.push_back({call->line, call->column,
                        "Function 'count_up' expects 3 arguments (input, reset, preset)"});
                } else {
                    if (!isBoolExpr(*call->args[0])) errors.push_back({call->args[0]->line, call->args[0]->column, "First argument of 'count_up' must be boolean"});
                    if (!isBoolExpr(*call->args[1])) errors.push_back({call->args[1]->line, call->args[1]->column, "Second argument of 'count_up' must be boolean"});
                    if (!isNumericOrTimeExpr(*call->args[2])) errors.push_back({call->args[2]->line, call->args[2]->column, "Third argument of 'count_up' must be integer"});
                }
            }
            else if (name == "count_down") {
                if (call->args.size() != 3) {
                    errors.push_back({call->line, call->column,
                        "Function 'count_down' expects 3 arguments (input, load, preset)"});
                } else {
                    if (!isBoolExpr(*call->args[0])) errors.push_back({call->args[0]->line, call->args[0]->column, "First argument of 'count_down' must be boolean"});
                    if (!isBoolExpr(*call->args[1])) errors.push_back({call->args[1]->line, call->args[1]->column, "Second argument of 'count_down' must be boolean"});
                    if (!isNumericOrTimeExpr(*call->args[2])) errors.push_back({call->args[2]->line, call->args[2]->column, "Third argument of 'count_down' must be integer"});
                }
            }
            else if (name == "count_updown") {
                if (call->args.size() != 5) {
                    errors.push_back({call->line, call->column,
                        "Function 'count_updown' expects 5 arguments (up, down, reset, load, preset)"});
                } else {
                    if (!isBoolExpr(*call->args[0])) errors.push_back({call->args[0]->line, call->args[0]->column, "First argument of 'count_updown' must be boolean"});
                    if (!isBoolExpr(*call->args[1])) errors.push_back({call->args[1]->line, call->args[1]->column, "Second argument of 'count_updown' must be boolean"});
                    if (!isBoolExpr(*call->args[2])) errors.push_back({call->args[2]->line, call->args[2]->column, "Third argument of 'count_updown' must be boolean"});
                    if (!isBoolExpr(*call->args[3])) errors.push_back({call->args[3]->line, call->args[3]->column, "Fourth argument of 'count_updown' must be boolean"});
                    if (!isNumericOrTimeExpr(*call->args[4])) errors.push_back({call->args[4]->line, call->args[4]->column, "Fifth argument of 'count_updown' must be integer"});
                }
            }
        }
        else {
            errors.push_back({call->line, call->column,
                "Unknown function '" + name + "'"});
        }
        // check sub-expressions in arguments
        for (const auto& arg : call->args) {
            checkExpr(*arg);
        }
    }
    else if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        checkExpr(*bin->left);
        checkExpr(*bin->right);
    }
    else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        checkExpr(*un->operand);
    }
    else if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        // Attribute access: for now just check that base object is defined? Not implemented.
        // We could later validate that object exists, but for now skip.
    }
}

//------------------------------------------------------------------------------
// Main Analysis Entry
//------------------------------------------------------------------------------
vector<SemanticError> SemanticAnalyzer::analyze(const Program& program) {
    errors.clear();

    for (const auto& func : program.functions) {
        for (const auto& stmt : func->body) {
            checkStmt(*stmt);
        }
    }

    return errors;
}