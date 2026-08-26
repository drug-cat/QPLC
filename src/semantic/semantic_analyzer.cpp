#include "semantic/semantic_analyzer.h"

#include "common/builtins.h"

#include <unordered_set>

using namespace std;

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

bool SemanticAnalyzer::isConstant(const string& name) const {
    return config.constants.find(name) != config.constants.end();
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

void SemanticAnalyzer::reportUndefined(const Expr& expr, const string& name) {
    errors.push_back({expr.line, expr.column,
        "Variable '" + name + "' is not defined in conf.qplc"});
}

//------------------------------------------------------------------------------
// Expression Type Checks
//------------------------------------------------------------------------------
bool SemanticAnalyzer::isBoolExpr(const Expr& expr) {
    if (auto var = dynamic_cast<const VarExpr*>(&expr)) {
        if (isLocalVar(var->name)) {
            // پارامتر تابع بی‌نوع است و در بافت بولی هم پذیرفته می‌شود؛
            // سایر متغیرهای محلی (مثل متغیر حلقه for) عددی‌اند
            return isActiveParam(var->name);
        }
        // ثابت True/False بولی است؛ سایر ثابت‌ها عددی/زمانی
        if (isConstant(var->name)) {
            return config.constants.at(var->name) == "True";
        }
        string t = getVarType(var->name);
        if (t.empty()) {
            reportUndefined(expr, var->name);
            return false;
        }
        return t == "BOOL";
    }
    if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        bool isArray = false;
        string t = getVarType(idx->name, isArray);
        if (t.empty()) {
            reportUndefined(expr, idx->name);
            return false;
        }
        if (!isArray) {
            errors.push_back({idx->line, idx->column,
                "Variable '" + idx->name + "' is not an array"});
            return false;
        }
        // بررسی حد آرایه اگر اندیس ثابت باشد
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
        const string canonical = builtins::normalize(call->funcName);
        // تایمرها، شمارنده‌ها و توابع لبه مقدار بولی برمی‌گردانند
        if (builtins::isTimer(canonical) || builtins::isCounter(canonical) ||
            builtins::isEdge(canonical)) {
            return true;
        }
        // توابع کاربر مقدار بازگشتی ندارند؛ خطا در checkExpr گزارش می‌شود
        return false;
    }
    if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        // فعلاً دسترسی به عضو (مثل timer1.Q) بولی فرض می‌شود
        return true;
    }
    if (dynamic_cast<const BoolExpr*>(&expr)) {
        return true;
    }
    if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (bin->op == "and" || bin->op == "or" || bin->op == "xor") {
            return isBoolExpr(*bin->left) && isBoolExpr(*bin->right);
        }
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" ||
            bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
            // مقایسه همیشه BOOL است
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
            // متغیر محلی/پارامتر عددی است
            return true;
        }
        // هر ثابت معتبری (به‌جز True/False) در بافت عددی قابل استفاده است
        if (isConstant(var->name)) {
            return true;
        }
        string t = getVarType(var->name);
        if (t.empty()) {
            reportUndefined(expr, var->name);
            return false;
        }
        return t != "BOOL";
    }
    if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        bool isArray = false;
        string t = getVarType(idx->name, isArray);
        if (t.empty()) {
            reportUndefined(expr, idx->name);
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
    if (dynamic_cast<const CallExpr*>(&expr)) {
        // تایمرها/شمارنده‌ها/لبه‌ها بولی هستند، نه عددی
        return false;
    }
    if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (bin->op == "+" || bin->op == "-" || bin->op == "*" ||
            bin->op == "/" || bin->op == "%") {
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
        if (isConstant(assign->name)) {
            errors.push_back({assign->line, assign->column,
                "Cannot assign to constant '" + assign->name + "'"});
            return;
        }
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
        if (isConstant(idxAssign->name)) {
            errors.push_back({idxAssign->line, idxAssign->column,
                "Cannot assign to constant '" + idxAssign->name + "'"});
            return;
        }
        bool isArray = false;
        string varType = getVarType(idxAssign->name, isArray);
        if (varType.empty()) {
            errors.push_back({idxAssign->line, idxAssign->column,
                "Variable '" + idxAssign->name + "' is not defined in conf.qplc"});
        } else if (!isArray) {
            errors.push_back({idxAssign->line, idxAssign->column,
                "Variable '" + idxAssign->name + "' is not an array"});
        } else {
            // بررسی حد آرایه اگر اندیس ثابت باشد
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
    else if (auto callStmt = dynamic_cast<const CallStmt*>(&stmt)) {
        checkUserCall(callStmt->funcName, static_cast<int>(callStmt->args.size()),
                      callStmt->line, callStmt->column);
    }
    else if (auto breakStmt = dynamic_cast<const BreakStmt*>(&stmt)) {
        if (loopDepth == 0) {
            errors.push_back({breakStmt->line, breakStmt->column,
                "'break' is only valid inside a while loop"});
        }
    }
    else if (auto continueStmt = dynamic_cast<const ContinueStmt*>(&stmt)) {
        if (loopDepth == 0) {
            errors.push_back({continueStmt->line, continueStmt->column,
                "'continue' is only valid inside a while loop"});
        }
    }
    else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        if (!isBoolExpr(*ifStmt->cond)) {
            errors.push_back({ifStmt->cond->line, ifStmt->cond->column,
                "Condition must be boolean"});
        }
        // دستورات مرکب داخل شاخه‌های if فعلاً پشتیبانی نمی‌شوند (ترکیب شرط شاخه با
        // کنترل جریان تودرتو در کد لدر ممکن نیست) — صریحاً رد می‌شوند
        auto rejectCompound = [this](const StmtPtr& s, const char* what) {
            if (dynamic_cast<const IfStmt*>(s.get()) || dynamic_cast<const WhileStmt*>(s.get()) ||
                dynamic_cast<const ForStmt*>(s.get()) || dynamic_cast<const CallStmt*>(s.get())) {
                errors.push_back({s->line, s->column,
                    string(what) + " cannot appear directly inside an if branch yet; restructure the program"});
            }
        };
        for (const auto& s : ifStmt->thenBlock) {
            rejectCompound(s, "this statement");
            checkStmt(*s);
        }
        for (const auto& branch : ifStmt->elifBranches) {
            if (!isBoolExpr(*branch.first)) {
                errors.push_back({branch.first->line, branch.first->column,
                    "Condition must be boolean"});
            }
            for (const auto& s : branch.second) {
                rejectCompound(s, "this statement");
                checkStmt(*s);
            }
        }
        for (const auto& s : ifStmt->elseBlock) {
            rejectCompound(s, "this statement");
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
        loopDepth++;
        for (const auto& innerStmt : forStmt->body) {
            checkStmt(*innerStmt);
        }
        loopDepth--;
        exitScope();
    }
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        if (!isBoolExpr(*whileStmt->cond)) {
            errors.push_back({whileStmt->cond->line, whileStmt->cond->column,
                "Condition must be boolean"});
        }
        loopDepth++;
        for (const auto& s : whileStmt->body) {
            checkStmt(*s);
        }
        loopDepth--;
    }
}

//------------------------------------------------------------------------------
// Expression Checking
//------------------------------------------------------------------------------
void SemanticAnalyzer::checkUserCall(const string& rawName, int argc, int line, int column) {
    const string name = builtins::normalize(rawName);

    if (builtins::isTimer(name)) {
        if (argc != 2) {
            errors.push_back({line, column,
                "Function '" + name + "' expects 2 arguments (input, time)"});
        }
        return;
    }
    if (builtins::isCounter(name)) {
        int expected = (name == "count_updown") ? 5 : 3;
        if (argc != expected) {
            errors.push_back({line, column,
                "Function '" + name + "' expects " + to_string(expected) + " arguments"});
        }
        return;
    }
    if (builtins::isEdge(name)) {
        if (argc != 1) {
            errors.push_back({line, column,
                "Function '" + name + "' expects 1 boolean argument"});
        }
        return;
    }
    // تابع کاربر
    if (declaredFunctions.find(name) == declaredFunctions.end()) {
        errors.push_back({line, column, "Unknown function '" + rawName + "'"});
    } else if (functionParamCounts.count(name) &&
               functionParamCounts.at(name) != argc) {
        errors.push_back({line, column,
            "Function '" + name + "' expects " + to_string(functionParamCounts.at(name)) +
            " arguments but " + to_string(argc) + " given"});
    }
}

void SemanticAnalyzer::checkExpr(const Expr& expr) {
    if (auto var = dynamic_cast<const VarExpr*>(&expr)) {
        if (config.io.find(var->name) == config.io.end() &&
            !isLocalVar(var->name) && !isConstant(var->name)) {
            errors.push_back({var->line, var->column,
                "Variable '" + var->name + "' is not defined in conf.qplc"});
        }
    }
    else if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        if (config.io.find(idx->name) == config.io.end() && !isConstant(idx->name)) {
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
        const string name = builtins::normalize(call->funcName);

        if (builtins::isTimer(name)) {
            // توابع تایمر: (BOOL, TIME)
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
        else if (builtins::isCounter(name)) {
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
            else {  // count_updown
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
        else if (builtins::isEdge(name)) {
            // توابع لبه: (BOOL) → BOOL
            if (call->args.size() != 1) {
                errors.push_back({call->line, call->column,
                    "Function '" + name + "' expects exactly 1 boolean argument"});
            } else if (!isBoolExpr(*call->args[0])) {
                errors.push_back({call->args[0]->line, call->args[0]->column,
                    "Argument of '" + name + "' must be boolean"});
            }
        }
        else {
            errors.push_back({call->line, call->column,
                "Unknown function '" + call->funcName +
                "' (user functions have no return value and cannot be used in expressions)"});
            // بررسی آرگومان‌ها برای یافتن خطاهای داخلی
            for (const auto& arg : call->args) {
                checkExpr(*arg);
            }
            return;
        }
        // بررسی زیرعبارت‌های آرگومان‌ها
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
        // دسترسی به عضو فعلاً بررسی نمی‌شود
    }
}

//------------------------------------------------------------------------------
// Main Analysis Entry
//------------------------------------------------------------------------------
vector<SemanticError> SemanticAnalyzer::analyze(const Program& program) {
    errors.clear();
    declaredFunctions.clear();
    functionParamCounts.clear();

    // ثبت امضای توابع (بررسی تکراری نبودن + تعداد پارامترها برای فراخوانی‌ها)
    for (const auto& func : program.functions) {
        if (declaredFunctions.count(func->name)) {
            errors.push_back({func->line, func->column,
                "Duplicate function definition '" + func->name + "'"});
            continue;
        }
        declaredFunctions.insert(func->name);
        functionParamCounts[func->name] = static_cast<int>(func->params.size());
    }

    // بررسی بدنه هر تابع با پارامترها به‌عنوان متغیر محلی بی‌نوع
    for (const auto& func : program.functions) {
        activeFunctionParams.clear();
        enterScope();
        for (const auto& param : func->params) {
            addLocalVar(param);
            activeFunctionParams.insert(param);
        }
        for (const auto& stmt : func->body) {
            checkStmt(*stmt);
        }
        exitScope();
    }
    activeFunctionParams.clear();

    return errors;
}
