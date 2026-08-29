#include "codegen/scl_generator.h"

#include "common/builtins.h"

#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <functional>

using namespace std;

namespace {

// ------------------------------------------------------------
// Helper: converts an AST expression to SCL text
// ------------------------------------------------------------
string resolveArrayAddress(const string& base, const string& type, const string& indexStr);

using SubstMap = unordered_map<string, const Expr*>;

// Helper: identity for string (with constant lookup)
static string exprToScl(const string& name, const Config* configPtr = nullptr) {
    if (configPtr) {
        auto cit = configPtr->constants.find(name);
        if (cit != configPtr->constants.end()) return cit->second;
    }
    // Try to resolve address from config
    if (configPtr) {
        auto it = configPtr->io.find(name);
        if (it != configPtr->io.end()) {
            return it->second.address;
        }
    }
    return name;
}

static string exprToScl(const Expr& expr, const Config* configPtr = nullptr) {
    if (auto num = dynamic_cast<const NumberExpr*>(&expr)) {
        return num->value;
    } else if (auto b = dynamic_cast<const BoolExpr*>(&expr)) {
        return b->value ? "TRUE" : "FALSE";
    } else if (auto t = dynamic_cast<const TimeExpr*>(&expr)) {
        // T#2s -> T#2S (SCL format)
        string v = t->value;
        if (v.rfind("T#", 0) == 0) {
            // Convert unit to uppercase: s->S, ms->MS, m->M, h->H
            string unit = v.substr(2);
            if (unit.size() >= 1) unit[0] = toupper(unit[0]);
            return "T#" + unit;
        }
        return v;
    } else if (auto v = dynamic_cast<const VarExpr*>(&expr)) {
        // Substitute constants defined in [constants]
        if (configPtr) {
            auto cit = configPtr->constants.find(v->name);
            if (cit != configPtr->constants.end()) return cit->second;
        }
        // Absolute addressing for SCL: %I0.0, %Q0.0, %MW10, %IW64
        if (configPtr) {
            auto it = configPtr->io.find(v->name);
            if (it != configPtr->io.end()) {
                return it->second.address; // absolute address from conf
            }
        }
        return v->name; // temporary variable/parameter
    } else if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        if (configPtr) {
            auto it = configPtr->io.find(idx->name);
            if (it != configPtr->io.end()) {
                string baseAddr = it->second.address; // e.g. %I0.0 or %MW10
                string type = it->second.type;
                string indexStr = exprToScl(*idx->index, configPtr);
                return resolveArrayAddress(baseAddr, type, indexStr);
            }
        }
        return idx->name + "[" + exprToScl(*idx->index) + "]";
    } else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        string op = un->op;
        if (op == "not") op = "NOT";
        else if (op == "-") op = "-";
        else if (op == "+") op = "+";
        return op + " " + exprToScl(*un->operand, configPtr);
    } else if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        string op = bin->op;
        if (op == "and") op = "AND";
        else if (op == "or") op = "OR";
        else if (op == "xor") op = "XOR";
        else if (op == "==") op = "=";
        else if (op == "!=") op = "<>";
        else if (op == ">=") op = ">=";
        else if (op == "<=") op = "<=";
        else if (op == ">") op = ">";
        else if (op == "<") op = "<";
        else if (op == "+") op = "+";
        else if (op == "-") op = "-";
        else if (op == "*") op = "*";
        else if (op == "/") op = "/";
        else if (op == "%") op = "MOD";
        return "(" + exprToScl(*bin->left, configPtr) + " " + op + " " + exprToScl(*bin->right, configPtr) + ")";
    } else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        const string canonical = builtins::normalize(call->funcName);
        if (builtins::isTimer(canonical)) {
            // TON/TOF/TP are called as pre-built instances in SCL (FB); not used as
            // an intrinsic function here; direct assignment is handled elsewhere
            return "(* TIMER INSTANCE CALL *)";
        }
        if (builtins::isCounter(canonical)) {
            return "(* COUNTER INSTANCE CALL *)";
        }
        if (builtins::isEdge(canonical)) {
            // R_TRIG / F_TRIG
            string edgeType = (canonical == "rising_edge") ? "R_TRIG" : "F_TRIG";
            if (call->args.size() == 1) {
                return edgeType + "(CLK := " + exprToScl(*call->args[0], configPtr) + ")";
            }
        }
        if (builtins::isMath(canonical)) {
            // IEC math: normalize QPLC name → IEC name
            string iecName = builtins::sclName(canonical);
            // LIMIT/SEL: swap arg[1] <-> arg[2] (lo/hi swap in IEC)
            if (canonical == "clamp" || canonical == "limit") {
                // clamp(x, lo, hi) → LIMIT(IN := x, MN := lo, MX := hi)
                // In IEC: LIMIT(IN:=x, MN:=lo, MX:=hi)
                string mn = (call->args.size() > 1) ? exprToScl(*call->args[1], configPtr) : "0";
                string mx = (call->args.size() > 2) ? exprToScl(*call->args[2], configPtr) : "0";
                string x  = (call->args.size() > 0) ? exprToScl(*call->args[0], configPtr) : "0";
                return "LIMIT(IN := " + x + ", MN := " + mn + ", MX := " + mx + ")";
            } else if (canonical == "sel") {
                string g = (call->args.size() > 0) ? exprToScl(*call->args[0], configPtr) : "FALSE";
                string a = (call->args.size() > 1) ? exprToScl(*call->args[1], configPtr) : "0";
                string b = (call->args.size() > 2) ? exprToScl(*call->args[2], configPtr) : "0";
                return "SEL(G := " + g + ", IN0 := " + a + ", IN1 := " + b + ")";
            } else if (canonical == "mux") {
                // MUX(K:=k, IN0:=v0, IN1:=v1, ...) — SCL native MUX
                string k = (call->args.size() > 0) ? exprToScl(*call->args[0], configPtr) : "0";
                string argsStr = "K := " + k;
                for (size_t i = 1; i < call->args.size(); ++i) {
                    argsStr += ", IN" + to_string(i - 1) + " := " + exprToScl(*call->args[i], configPtr);
                }
                return "MUX(" + argsStr + ")";
            } else {
                // MIN/MAX/ABS — direct call
                string argsStr;
                for (size_t i = 0; i < call->args.size(); ++i) {
                    if (i > 0) argsStr += ", ";
                    argsStr += exprToScl(*call->args[i], configPtr);
                }
                return iecName + "(" + argsStr + ")";
            }
        }
        // User functions are called in SCL
        string argsStr;
        for (size_t i = 0; i < call->args.size(); ++i) {
            if (i > 0) argsStr += ", ";
            argsStr += exprToScl(*call->args[i], configPtr);
        }
        return call->funcName + "(" + argsStr + ")";
    } else if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        // In SCL, member access such as timer1.Q is not supported; instance.Q is used instead
        return attr->objectName + "." + attr->attrName;
    }
    return "?";
}

// ------------------------------------------------------------
// Converts an array address to an absolute SCL address
// ------------------------------------------------------------
string resolveArrayAddress(const string& base, const string& type, const string& indexStr) {
    int index = stoi(indexStr);
    size_t dot = base.rfind('.');
    if (dot != string::npos) {
        // Bit address such as %I0.0
        if (type != "BOOL") return base;
        string prefix = base.substr(0, dot);
        int bit = stoi(base.substr(dot+1));
        bit += index;
        return prefix + "." + to_string(bit);
    } else {
        // Word/double-word address such as %MW10
        size_t numStart = base.find_last_not_of("0123456789");
        if (numStart == string::npos) return base;
        numStart++;
        string prefix = base.substr(0, numStart);
        string numStr = base.substr(numStart);
        int addr = stoi(numStr);
        int stride = 1;
        if (type == "INT") stride = 1;       // WORD: 1 word per element
        else if (type == "REAL" || type == "TIME" || type == "DINT") stride = 2; // DWORD: 2 words
        addr += index * stride;
        return prefix + to_string(addr);
    }
}

// ------------------------------------------------------------
// Escape for SCL strings
// ------------------------------------------------------------
string escapeScl(const string& input) {
    string out = input;
    for (size_t i = 0; i < out.size(); ++i) {
        if (out[i] == '\'') {
            out.insert(i, 1, '\'');
            ++i;
        }
    }
    return out;
}

// ------------------------------------------------------------
// Clones an expression, replacing parameters (for user functions)
// ------------------------------------------------------------
ExprPtr cloneExprWithSubst(const Expr& expr, const SubstMap& subs) {
    if (auto num = dynamic_cast<const NumberExpr*>(&expr)) {
        return make_unique<NumberExpr>(num->value, num->isFloat, num->line, num->column);
    }
    if (auto b = dynamic_cast<const BoolExpr*>(&expr)) {
        return make_unique<BoolExpr>(b->value, b->line, b->column);
    }
    if (auto t = dynamic_cast<const TimeExpr*>(&expr)) {
        return make_unique<TimeExpr>(t->value, t->line, t->column);
    }
    if (auto var = dynamic_cast<const VarExpr*>(&expr)) {
        auto it = subs.find(var->name);
        if (it != subs.end() && it->second) {
            return cloneExprWithSubst(*it->second, subs);
        }
        return make_unique<VarExpr>(var->name, var->line, var->column);
    }
    if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        ExprPtr clonedIndex = cloneExprWithSubst(*idx->index, subs);
        return make_unique<IndexExpr>(idx->name, std::move(clonedIndex), idx->line, idx->column);
    }
    if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        vector<ExprPtr> clonedArgs;
        for (const auto& arg : call->args) {
            clonedArgs.push_back(cloneExprWithSubst(*arg, subs));
        }
        return make_unique<CallExpr>(call->funcName, std::move(clonedArgs), call->line, call->column);
    }
    if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        return make_unique<AttributeExpr>(attr->objectName, attr->attrName, attr->line, attr->column);
    }
    if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        ExprPtr clonedOperand = cloneExprWithSubst(*un->operand, subs);
        return make_unique<UnaryExpr>(un->op, std::move(clonedOperand), un->line, un->column);
    }
    if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        ExprPtr left = cloneExprWithSubst(*bin->left, subs);
        ExprPtr right = cloneExprWithSubst(*bin->right, subs);
        return make_unique<BinaryExpr>(bin->op, std::move(left), std::move(right), bin->line, bin->column);
    }
    return nullptr;
}

StmtPtr cloneStmtWithSubst(const Stmt& stmt, const SubstMap& subs) {
    if (auto assign = dynamic_cast<const AssignmentStmt*>(&stmt)) {
        ExprPtr expr = cloneExprWithSubst(*assign->expr, subs);
        return make_unique<AssignmentStmt>(assign->name, std::move(expr), assign->line, assign->column);
    }
    if (auto idxAssign = dynamic_cast<const IndexAssignmentStmt*>(&stmt)) {
        ExprPtr index = cloneExprWithSubst(*idxAssign->index, subs);
        ExprPtr expr = cloneExprWithSubst(*idxAssign->expr, subs);
        return make_unique<IndexAssignmentStmt>(idxAssign->name, std::move(index), std::move(expr), idxAssign->line, idxAssign->column);
    }
    if (auto callStmt = dynamic_cast<const CallStmt*>(&stmt)) {
        vector<ExprPtr> clonedArgs;
        for (const auto& arg : callStmt->args) {
            clonedArgs.push_back(cloneExprWithSubst(*arg, subs));
        }
        return make_unique<CallStmt>(callStmt->funcName, std::move(clonedArgs), callStmt->line, callStmt->column);
    }
    if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        ExprPtr cond = cloneExprWithSubst(*ifStmt->cond, subs);
        vector<StmtPtr> thenBlock;
        for (const auto& s : ifStmt->thenBlock) thenBlock.push_back(cloneStmtWithSubst(*s, subs));
        vector<pair<ExprPtr, vector<StmtPtr>>> elifBranches;
        for (const auto& branch : ifStmt->elifBranches) {
            ExprPtr elifCond = cloneExprWithSubst(*branch.first, subs);
            vector<StmtPtr> elifBlock;
            for (const auto& s : branch.second) elifBlock.push_back(cloneStmtWithSubst(*s, subs));
            elifBranches.emplace_back(std::move(elifCond), std::move(elifBlock));
        }
        vector<StmtPtr> elseBlock;
        for (const auto& s : ifStmt->elseBlock) elseBlock.push_back(cloneStmtWithSubst(*s, subs));
        return make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elifBranches), std::move(elseBlock), ifStmt->line, ifStmt->column);
    }
    if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        vector<StmtPtr> body;
        for (const auto& s : forStmt->body) body.push_back(cloneStmtWithSubst(*s, subs));
        return make_unique<ForStmt>(forStmt->varName, forStmt->start, forStmt->end, std::move(body), forStmt->line, forStmt->column);
    }
    if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        ExprPtr cond = cloneExprWithSubst(*whileStmt->cond, subs);
        vector<StmtPtr> body;
        for (const auto& s : whileStmt->body) body.push_back(cloneStmtWithSubst(*s, subs));
        return make_unique<WhileStmt>(std::move(cond), std::move(body), whileStmt->line, whileStmt->column);
    }
    return nullptr;
}

// ------------------------------------------------------------
// Collects temporary variables (VAR_TEMP) and timer/counter instances
// ------------------------------------------------------------
struct SclVariable {
    string name;
    string type;      // BOOL, INT, REAL, TIME, TON, TOF, TP, CTU, CTD, CTUD, R_TRIG, F_TRIG
    string address;   // absolute address (for IO)
    bool isTemp = false;
    bool isInstance = false;
};

void collectVariables(const Program& program, const Config& config,
                      vector<SclVariable>& tempVars,
                      vector<SclVariable>& instances,
                      unordered_map<string, string>& timerTypeOf,
                      unordered_map<string, string>& counterTypeOf) {
    tempVars.clear();
    instances.clear();
    timerTypeOf.clear();
    counterTypeOf.clear();

    // IO variables from config
    for (const auto& kv : config.io) {
        SclVariable v;
        v.name = kv.first;
        v.type = kv.second.type;
        v.address = kv.second.address;
        v.isTemp = false;
        tempVars.push_back(v); // in SCL, IO variables are defined in VAR_GLOBAL or VAR in FB
    }

    // Walk the AST to find timers, counters, and edges
    function<void(const Stmt&)> walk;
    walk = [&](const Stmt& stmt) {
        if (auto assign = dynamic_cast<const AssignmentStmt*>(&stmt)) {
            if (auto call = dynamic_cast<const CallExpr*>(assign->expr.get())) {
                string canon = builtins::normalize(call->funcName);
                if (builtins::isTimer(canon)) {
                    timerTypeOf[assign->name] = canon;
                    SclVariable inst;
                    inst.name = assign->name + "_" + canon; // e.g. motor_run_TON
                    inst.type = (canon == "on_delay") ? "TON" : (canon == "off_delay") ? "TOF" : "TP";
                    inst.isInstance = true;
                    instances.push_back(inst);
                } else if (builtins::isCounter(canon)) {
                    counterTypeOf[assign->name] = canon;
                    SclVariable inst;
                    inst.name = assign->name + "_" + canon; // e.g. done_CTU
                    inst.type = (canon == "count_up") ? "CTU" : (canon == "count_down") ? "CTD" : "CTUD";
                    inst.isInstance = true;
                    instances.push_back(inst);
                }
            }
        }
        else if (auto idxAssign = dynamic_cast<const IndexAssignmentStmt*>(&stmt)) {
            if (auto call = dynamic_cast<const CallExpr*>(idxAssign->expr.get())) {
                string canon = builtins::normalize(call->funcName);
                // Array address cannot be computed here; ignored for simplicity
            }
        }
        else if (auto callStmt = dynamic_cast<const CallStmt*>(&stmt)) {
            // User function calls — variable collection is handled recursively in the expr walker
        }
        else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
            for (const auto& s : ifStmt->thenBlock) walk(*s);
            for (const auto& b : ifStmt->elifBranches) for (const auto& s : b.second) walk(*s);
            for (const auto& s : ifStmt->elseBlock) walk(*s);
        }
        else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
            for (const auto& s : whileStmt->body) walk(*s);
        }
        else if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
            for (const auto& s : forStmt->body) walk(*s);
        }
    };

    for (const auto& func : program.functions) {
        if (func->name != "main") continue;
        for (const auto& s : func->body) walk(*s);
    }

    // Edges: R_TRIG / F_TRIG variables
    function<void(const Expr&)> walkExpr;
    walkExpr = [&](const Expr& expr) {
        if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
            string canon = builtins::normalize(call->funcName);
            if (builtins::isEdge(canon)) {
                // Temporary variable for edge detection
                string edgeName = "EDGE_" + exprToScl(*call->args[0], &config);
                SclVariable inst;
                inst.name = edgeName;
                inst.type = (canon == "rising_edge") ? "R_TRIG" : "F_TRIG";
                inst.isInstance = true;
                // Avoid duplicates
                bool exists = false;
                for (auto& iv : instances) if (iv.name == inst.name) { exists = true; break; }
                if (!exists) instances.push_back(inst);
            }
        }
        if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
            walkExpr(*bin->left);
            walkExpr(*bin->right);
        }
        else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
            walkExpr(*un->operand);
        }
        else if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
            walkExpr(*idx->index);
        }
        else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
            for (auto& a : call->args) walkExpr(*a);
        }
    };

    function<void(const Stmt&)> walkExprStmt = [&](const Stmt& stmt) {
        if (auto assign = dynamic_cast<const AssignmentStmt*>(&stmt)) {
            walkExpr(*assign->expr);
        }
        else if (auto idxAssign = dynamic_cast<const IndexAssignmentStmt*>(&stmt)) {
            walkExpr(*idxAssign->expr);
            walkExpr(*idxAssign->index);
        }
        else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
            walkExpr(*ifStmt->cond);
            for (auto& s : ifStmt->thenBlock) walkExprStmt(*s);
            for (auto& b : ifStmt->elifBranches) {
                walkExpr(*b.first);
                for (auto& s : b.second) walkExprStmt(*s);
            }
            for (auto& s : ifStmt->elseBlock) walkExprStmt(*s);
        }
        else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
            walkExpr(*whileStmt->cond);
            for (auto& s : whileStmt->body) walkExprStmt(*s);
        }
        else if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
            for (auto& s : forStmt->body) walkExprStmt(*s);
        }
    };

    for (const auto& func : program.functions) {
        if (func->name != "main") continue;
        for (const auto& s : func->body) walkExprStmt(*s);
    }
}

// ------------------------------------------------------------
// Generates SCL for a statement
// ------------------------------------------------------------
void generateSclStmt(ostream& out, const Stmt& stmt, const Config& config, int indent,
                     const unordered_map<string, string>& timerTypeOf,
                     const unordered_map<string, string>& counterTypeOf,
                     const unordered_map<string, const FunctionDef*>& funcMap,
                     int& callDepth);

void generateSclBlock(ostream& out, const vector<StmtPtr>& stmts, const Config& config, int indent,
                      const unordered_map<string, string>& timerTypeOf,
                      const unordered_map<string, string>& counterTypeOf,
                      const unordered_map<string, const FunctionDef*>& funcMap,
                      int& callDepth) {
    for (const auto& s : stmts) {
        generateSclStmt(out, *s, config, indent, timerTypeOf, counterTypeOf, funcMap, callDepth);
    }
}

void generateSclStmt(ostream& out, const Stmt& stmt, const Config& config, int indent,
                     const unordered_map<string, string>& timerTypeOf,
                     const unordered_map<string, string>& counterTypeOf,
                     const unordered_map<string, const FunctionDef*>& funcMap,
                     int& callDepth) {
    string ind(indent, ' ');

    if (auto assign = dynamic_cast<const AssignmentStmt*>(&stmt)) {
        auto itTimer = timerTypeOf.find(assign->name);
        auto itCounter = counterTypeOf.find(assign->name);
        if (itTimer != timerTypeOf.end()) {
            // Timer instance call in SCL:
            //   motor_run_TON(IN := %I0.0, PT := T#2S);
            //   %Q0.0 := motor_run_TON.Q;
            if (auto call = dynamic_cast<const CallExpr*>(assign->expr.get())) {
                string instName = assign->name + "_" + itTimer->second;
                string inputArg = (call->args.size() >= 1) ? exprToScl(*call->args[0], &config) : "FALSE";
                string ptArg = (call->args.size() >= 2) ? exprToScl(*call->args[1], &config) : "T#0S";
                out << ind << instName << "(IN := " << inputArg << ", PT := " << ptArg << ");\n";
                out << ind << exprToScl(assign->name, &config) << " := " << instName << ".Q;\n";
            }
            return;
        }
        if (itCounter != counterTypeOf.end()) {
            // Counter instance call in SCL
            if (auto call = dynamic_cast<const CallExpr*>(assign->expr.get())) {
                string instName = assign->name + "_" + itCounter->second;
                const auto& canon = itCounter->second;
                vector<string> argStrs;
                for (const auto& arg : call->args) argStrs.push_back(exprToScl(*arg, &config));
                out << ind << instName << "(";
                if (canon == "count_up" && argStrs.size() >= 3) {
                    out << "CU := " << argStrs[0] << ", R := " << argStrs[1] << ", PV := " << argStrs[2];
                } else if (canon == "count_down" && argStrs.size() >= 3) {
                    out << "CD := " << argStrs[0] << ", LD := " << argStrs[1] << ", PV := " << argStrs[2];
                } else if (canon == "count_updown" && argStrs.size() >= 5) {
                    out << "CU := " << argStrs[0] << ", CD := " << argStrs[1]
                        << ", R := " << argStrs[2] << ", LD := " << argStrs[3] << ", PV := " << argStrs[4];
                }
                out << ");\n";
                out << ind << exprToScl(assign->name, &config) << " := " << instName << ".Q;\n";
            }
            return;
        }
        out << ind << exprToScl(assign->name, &config) << " := " << exprToScl(*assign->expr, &config) << ";\n";
    }
    else if (auto idxAssign = dynamic_cast<const IndexAssignmentStmt*>(&stmt)) {
        // Compute absolute address of the array element
        string addr;
        auto itIO = config.io.find(idxAssign->name);
        if (itIO != config.io.end()) {
            string indexStr = exprToScl(*idxAssign->index, &config);
            addr = resolveArrayAddress(itIO->second.address, itIO->second.type, indexStr);
        } else {
            addr = idxAssign->name + "[" + exprToScl(*idxAssign->index, &config) + "]";
        }
        out << ind << addr << " := " << exprToScl(*idxAssign->expr, &config) << ";\n";
    }
    else if (auto callStmt = dynamic_cast<const CallStmt*>(&stmt)) {
        // Call user functions
        out << ind << callStmt->funcName << "(";
        for (size_t i = 0; i < callStmt->args.size(); ++i) {
            if (i > 0) out << ", ";
            out << exprToScl(*callStmt->args[i], &config);
        }
        out << ");\n";
    }
    else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        out << ind << "IF " << exprToScl(*ifStmt->cond, &config) << " THEN\n";
        generateSclBlock(out, ifStmt->thenBlock, config, indent + 4, timerTypeOf, counterTypeOf, funcMap, callDepth);
        for (const auto& branch : ifStmt->elifBranches) {
            out << ind << "ELSIF " << exprToScl(*branch.first, &config) << " THEN\n";
            generateSclBlock(out, branch.second, config, indent + 4, timerTypeOf, counterTypeOf, funcMap, callDepth);
        }
        if (!ifStmt->elseBlock.empty()) {
            out << ind << "ELSE\n";
            generateSclBlock(out, ifStmt->elseBlock, config, indent + 4, timerTypeOf, counterTypeOf, funcMap, callDepth);
        }
        out << ind << "END_IF;\n";
    }
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        out << ind << "WHILE " << exprToScl(*whileStmt->cond, &config) << " DO\n";
        generateSclBlock(out, whileStmt->body, config, indent + 4, timerTypeOf, counterTypeOf, funcMap, callDepth);
        out << ind << "END_WHILE;\n";
    }
    else if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        // Like ladder, the for loop is unrolled because absolute SCL addressing
        // does not support a variable index
        SubstMap subs;
        for (int i = forStmt->start; i < forStmt->end; ++i) {
            NumberExpr loopValue(to_string(i), false, forStmt->line, forStmt->column);
            subs[forStmt->varName] = &loopValue;
            vector<StmtPtr> clonedBody;
            for (const auto& inner : forStmt->body) {
                StmtPtr cloned = cloneStmtWithSubst(*inner, subs);
                if (cloned) generateSclStmt(out, *cloned, config, indent,
                                            timerTypeOf, counterTypeOf, funcMap, callDepth);
            }
        }
    }
    else if (dynamic_cast<const BreakStmt*>(&stmt)) {
        out << ind << "EXIT;\n"; // SCL equivalent of break
    }
    else if (dynamic_cast<const ContinueStmt*>(&stmt)) {
        out << ind << "CONTINUE;\n"; // SCL equivalent of continue
    }
}

// ------------------------------------------------------------
// User functions: expansion and output
// ------------------------------------------------------------
void generateUserFunction(ostream& out, const FunctionDef& func, const Config& config,
                          const unordered_map<string, string>& timerTypeOf,
                          const unordered_map<string, string>& counterTypeOf,
                          const unordered_map<string, const FunctionDef*>& funcMap) {
    if (func.name == "main") return; // main is in the main FB

    out << "\nFUNCTION " << func.name << " : VOID\n";
    out << "VAR_INPUT\n";
    for (const auto& param : func.params) {
        out << "    " << param << " : BOOL; (* TODO: type inference *)\n";
    }
    out << "END_VAR\n";
    out << "VAR\n";
    // Temporary variables of user functions
    out << "END_VAR\n\n";

    int callDepth = 0;
    generateSclBlock(out, func.body, config, 4, timerTypeOf, counterTypeOf, funcMap, callDepth);

    out << "END_FUNCTION\n";
}

} // namespace

// ------------------------------------------------------------
// Main SCL generator
// ------------------------------------------------------------
string generateScl(const Program& program, const Config& config) {
    // Collect variables and instances
    vector<SclVariable> tempVars;
    vector<SclVariable> instances;
    unordered_map<string, string> timerTypeOf;
    unordered_map<string, string> counterTypeOf;
    collectVariables(program, config, tempVars, instances, timerTypeOf, counterTypeOf);

    // Map of user functions
    unordered_map<string, const FunctionDef*> funcMap;
    for (const auto& func : program.functions) {
        if (!funcMap.count(func->name)) {
            funcMap[func->name] = func.get();
        }
    }

    stringstream out;

    // SCL file header
    out << "// Generated by QPLC compiler for TIA Portal V19\n";
    out << "// Structured Control Language (SCL) source\n";
    out << "// Import: Options > External Files > Import in TIA Portal\n\n";

    // Main Function Block
    out << "FUNCTION_BLOCK \"QPLC_Main\"\n";
    out << "{ S7_Optimized_Access := 'TRUE' }\n";
    out << "VAR\n";

    // Input/output variables (from config)
    for (const auto& v : tempVars) {
        if (!v.isInstance) {
            string addrComment = v.address.empty() ? "" : " (* AT " + v.address + " *)";
            out << "    " << v.name << " : " << v.type << addrComment << ";\n";
        }
    }

    out << "END_VAR\n\n";

    out << "VAR_TEMP\n";
    // Timer/counter/edge instances
    for (const auto& inst : instances) {
        out << "    " << inst.name << " : " << inst.type << ";\n";
    }
    out << "END_VAR\n\n";

    out << "BEGIN\n\n";

    // Main body
    int callDepth = 0;
    for (const auto& func : program.functions) {
        if (func->name != "main") continue;
        generateSclBlock(out, func->body, config, 4, timerTypeOf, counterTypeOf, funcMap, callDepth);
    }

    out << "\nEND_FUNCTION_BLOCK\n\n";

    // User functions
    for (const auto& func : program.functions) {
        if (func->name != "main") {
            generateUserFunction(out, *func, config, timerTypeOf, counterTypeOf, funcMap);
        }
    }

    return out.str();
}
