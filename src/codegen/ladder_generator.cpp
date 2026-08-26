#include "codegen/ladder_generator.h"

#include "common/builtins.h"

#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <functional>
#include <cctype>
#include <cstdlib>
#include <unordered_set>
#include <unordered_map>

using namespace std;

namespace {

// ------------------- Expression to String -------------------
string resolveArrayAddress(const string& base, const string& type, const string& indexStr);

using SubstMap = unordered_map<string, const Expr*>;

string exprToString(const Expr& expr, const Config* configPtr = nullptr) {
    if (auto num = dynamic_cast<const NumberExpr*>(&expr)) {
        return num->value;
    } else if (auto b = dynamic_cast<const BoolExpr*>(&expr)) {
        return b->value ? "TRUE" : "FALSE";
    } else if (auto t = dynamic_cast<const TimeExpr*>(&expr)) {
        return t->value;
    } else if (auto v = dynamic_cast<const VarExpr*>(&expr)) {
        // جایگزینی ثابت تعریف‌شده در [constants] با مقدار خام آن
        if (configPtr) {
            auto cit = configPtr->constants.find(v->name);
            if (cit != configPtr->constants.end()) return cit->second;
        }
        return v->name;
    } else if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        if (configPtr) {
            auto it = configPtr->io.find(idx->name);
            if (it != configPtr->io.end()) {
                string baseAddr = it->second.address;
                string type = it->second.type;
                string indexStr = exprToString(*idx->index, configPtr);
                return resolveArrayAddress(baseAddr, type, indexStr);
            }
        }
        return idx->name + "[" + exprToString(*idx->index) + "]";
    } else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        return un->op + "(" + exprToString(*un->operand, configPtr) + ")";
    } else if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        return "(" + exprToString(*bin->left, configPtr) + " " + bin->op + " " + exprToString(*bin->right, configPtr) + ")";
    } else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        string argsStr;
        for (size_t i = 0; i < call->args.size(); ++i) {
            if (i > 0) argsStr += ", ";
            argsStr += exprToString(*call->args[i], configPtr);
        }
        return call->funcName + "(" + argsStr + ")";
    } else if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        return attr->objectName + "." + attr->attrName;
    }
    return "?";
}

// ------------------- Contact Representation -------------------
struct Contact {
    enum class Kind { VAR, COMP, EDGE };
    Kind kind;
    string name;      // VAR: آدرس متغیر، EDGE: عملوند لبه
    string op;        // COMP: عملگر مقایسه؛ EDGE: "rising" یا "falling"
    string left;
    string right;
    bool negated;

    Contact(Kind k, string n, string o, string l, string r, bool neg)
        : kind(k), name(move(n)), op(move(o)), left(move(l)), right(move(r)), negated(neg) {}

    static Contact makeVar(const string& varName, bool isNegated) {
        return Contact(Kind::VAR, varName, "", "", "", isNegated);
    }

    static Contact makeComp(const string& op, const string& left, const string& right, bool isNegated) {
        string newOp = op;
        if (isNegated) {
            if (op == "eq") newOp = "ne";
            else if (op == "ne") newOp = "eq";
            else if (op == "gt") newOp = "le";
            else if (op == "lt") newOp = "ge";
            else if (op == "ge") newOp = "lt";
            else if (op == "le") newOp = "gt";
        }
        return Contact(Kind::COMP, "", newOp, left, right, false);
    }

    static Contact makeEdge(const string& edgeType, const string& operand) {
        // edgeType: "rising" یا "falling"
        return Contact(Kind::EDGE, operand, edgeType, "", "", false);
    }
};

struct Term {
    vector<Contact> contacts;
};

string normalizeOp(const string& op) {
    if (op == "==") return "eq";
    if (op == "!=") return "ne";
    if (op == "<")  return "lt";
    if (op == ">")  return "gt";
    if (op == "<=") return "le";
    if (op == ">=") return "ge";
    return op;
}

string flipEdge(const string& edgeType) {
    return edgeType == "rising" ? "falling" : "rising";
}

string resolveArrayAddress(const string& base, const string& type, const string& indexStr) {
    int index = stoi(indexStr);
    size_t dot = base.rfind('.');
    if (dot != string::npos) {
        if (type != "BOOL") return base;
        string prefix = base.substr(0, dot);
        int bit = stoi(base.substr(dot+1));
        bit += index;
        return prefix + "." + to_string(bit);
    } else {
        size_t numStart = base.find_last_not_of("0123456789");
        if (numStart == string::npos) return base;
        numStart++;
        string prefix = base.substr(0, numStart);
        string numStr = base.substr(numStart);
        int addr = stoi(numStr);
        int stride = 1;
        if (type == "INT") stride = 2;
        else if (type == "REAL" || type == "TIME" || type == "DINT") stride = 4;
        addr += index * stride;
        return prefix + to_string(addr);
    }
}

vector<Term> exprToDNF(const Expr& expr, const Config& config);

vector<Term> negateDNF(const vector<Term>& dnf) {
    vector<vector<Contact>> cnf;
    for (const auto& term : dnf) {
        vector<Contact> clause;
        for (const auto& contact : term.contacts) {
            if (contact.kind == Contact::Kind::VAR) {
                clause.push_back(Contact::makeVar(contact.name, !contact.negated));
            } else if (contact.kind == Contact::Kind::COMP) {
                string op = contact.op;
                string flippedOp;
                if (op == "eq") flippedOp = "ne";
                else if (op == "ne") flippedOp = "eq";
                else if (op == "gt") flippedOp = "le";
                else if (op == "lt") flippedOp = "ge";
                else if (op == "ge") flippedOp = "lt";
                else if (op == "le") flippedOp = "gt";
                clause.push_back(Contact::makeComp(flippedOp, contact.left, contact.right, false));
            } else {  // EDGE: نقیض لبه صعودی، لبه نزولی است و برعکس
                clause.push_back(Contact::makeEdge(flipEdge(contact.op), contact.name));
            }
        }
        cnf.push_back(move(clause));
    }

    vector<Term> result;
    if (cnf.empty()) {
        Term alwaysTrue;
        result.push_back(move(alwaysTrue));
        return result;
    }

    const auto& firstClause = cnf[0];
    vector<Term> current;
    for (const auto& contact : firstClause) {
        Term term;
        term.contacts.push_back(contact);
        current.push_back(move(term));
    }

    for (size_t i = 1; i < cnf.size(); ++i) {
        const auto& clause = cnf[i];
        vector<Term> newCurrent;
        for (const auto& term : current) {
            for (const auto& contact : clause) {
                Term newTerm = term;
                newTerm.contacts.push_back(contact);
                newCurrent.push_back(move(newTerm));
            }
        }
        current = move(newCurrent);
    }
    result = move(current);
    return result;
}

vector<Term> exprToDNF(const Expr& expr, const Config& config) {
    vector<Term> result;

    if (auto var = dynamic_cast<const VarExpr*>(&expr)) {
        Term term;
        term.contacts.push_back(Contact::makeVar(var->name, false));
        result.push_back(move(term));
    }
    else if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        auto it = config.io.find(idx->name);
        if (it != config.io.end()) {
            string baseAddr = it->second.address;
            string type = it->second.type;
            string indexStr = exprToString(*idx->index, &config);
            string addr = resolveArrayAddress(baseAddr, type, indexStr);
            Term term;
            term.contacts.push_back(Contact::makeVar(addr, false));
            result.push_back(move(term));
        }
    }
    else if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
            Term term;
            string op = normalizeOp(bin->op);
            string leftStr = exprToString(*bin->left, &config);
            string rightStr = exprToString(*bin->right, &config);
            term.contacts.push_back(Contact::makeComp(op, leftStr, rightStr, false));
            result.push_back(move(term));
        }
        else if (bin->op == "and") {
            auto left = exprToDNF(*bin->left, config);
            auto right = exprToDNF(*bin->right, config);
            for (auto& l : left) {
                for (auto& r : right) {
                    Term term = l;
                    term.contacts.insert(term.contacts.end(), r.contacts.begin(), r.contacts.end());
                    result.push_back(move(term));
                }
            }
        }
        else if (bin->op == "or") {
            auto left = exprToDNF(*bin->left, config);
            auto right = exprToDNF(*bin->right, config);
            result = move(left);
            result.insert(result.end(), right.begin(), right.end());
        }
        else if (bin->op == "xor") {
            // XOR: (a ∧ ¬b) ∨ (¬a ∧ b)
            auto left = exprToDNF(*bin->left, config);
            auto right = exprToDNF(*bin->right, config);
            auto notLeft = negateDNF(left);
            auto notRight = negateDNF(right);
            for (auto& l : left) {
                for (auto& nr : notRight) {
                    Term term = l;
                    term.contacts.insert(term.contacts.end(), nr.contacts.begin(), nr.contacts.end());
                    result.push_back(move(term));
                }
            }
            for (auto& nl : notLeft) {
                for (auto& r : right) {
                    Term term = nl;
                    term.contacts.insert(term.contacts.end(), r.contacts.begin(), r.contacts.end());
                    result.push_back(move(term));
                }
            }
        }
    }
    else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (un->op == "not") {
            auto innerVar = dynamic_cast<const VarExpr*>(un->operand.get());
            if (innerVar) {
                Term term;
                term.contacts.push_back(Contact::makeVar(innerVar->name, true));
                result.push_back(move(term));
            } else {
                auto innerIdx = dynamic_cast<const IndexExpr*>(un->operand.get());
                if (innerIdx) {
                    auto it = config.io.find(innerIdx->name);
                    if (it != config.io.end()) {
                        string baseAddr = it->second.address;
                        string type = it->second.type;
                        string indexStr = exprToString(*innerIdx->index, &config);
                        string addr = resolveArrayAddress(baseAddr, type, indexStr);
                        Term term;
                        term.contacts.push_back(Contact::makeVar(addr, true));
                        result.push_back(move(term));
                    }
                } else {
                    auto innerBin = dynamic_cast<const BinaryExpr*>(un->operand.get());
                    if (innerBin && (innerBin->op == "==" || innerBin->op == "!=" || innerBin->op == "<" || innerBin->op == ">" || innerBin->op == "<=" || innerBin->op == ">=")) {
                        Term term;
                        string op = normalizeOp(innerBin->op);
                        string leftStr = exprToString(*innerBin->left, &config);
                        string rightStr = exprToString(*innerBin->right, &config);
                        term.contacts.push_back(Contact::makeComp(op, leftStr, rightStr, true));
                        result.push_back(move(term));
                    } else {
                        auto innerCall = dynamic_cast<const CallExpr*>(un->operand.get());
                        if (innerCall) {
                            const string canonical = builtins::normalize(innerCall->funcName);
                            if (builtins::isEdge(canonical)) {
                                // ¬rising_edge(x) ≡ falling_edge(x)
                                string flipped = (canonical == "rising_edge") ? "falling" : "rising";
                                string addr = exprToString(*innerCall->args[0], &config);
                                Term term;
                                term.contacts.push_back(Contact::makeEdge(flipped, addr));
                                result.push_back(move(term));
                            }
                        }
                    }
                }
            }
        }
    }
    else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        const string canonical = builtins::normalize(call->funcName);
        if (builtins::isEdge(canonical) && call->args.size() == 1) {
            // کنتاکت لبه به‌صورت بومی در XML
            string edgeType = (canonical == "rising_edge") ? "rising" : "falling";
            string addr = exprToString(*call->args[0], &config);
            Term term;
            term.contacts.push_back(Contact::makeEdge(edgeType, addr));
            result.push_back(move(term));
        } else {
            // سایر فراخوانی‌ها (تایمر/شمارنده خارج از انتساب مستقیم): کنتاکت متنی
            Term term;
            string callStr = exprToString(expr, &config);
            term.contacts.push_back(Contact::makeVar(callStr, false));
            result.push_back(move(term));
        }
    }
    else if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        Term term;
        string attrStr = exprToString(expr, &config);
        term.contacts.push_back(Contact::makeVar(attrStr, false));
        result.push_back(move(term));
    }
    else if (auto b = dynamic_cast<const BoolExpr*>(&expr)) {
        if (b->value) {
            Term term;
            result.push_back(move(term));
        }
    }

    return result;
}

string escapeXml(const string& input) {
    string out = input;
    for (size_t i = 0; i < out.size(); ++i) {
        char c = out[i];
        switch (c) {
            case '&': out.replace(i, 1, "&amp;"); i += 4; break;
            case '<': out.replace(i, 1, "&lt;"); i += 3; break;
            case '>': out.replace(i, 1, "&gt;"); i += 3; break;
            case '"': out.replace(i, 1, "&quot;"); i += 5; break;
            case '\'': out.replace(i, 1, "&apos;"); i += 5; break;
        }
    }
    return out;
}

// نوشتن یک زنجیره کنتاکت (نقطه مشترک همه انواع رانگ‌ها)
void writeContacts(ostream& ss, const vector<Contact>& contacts) {
    for (const auto& contact : contacts) {
        switch (contact.kind) {
            case Contact::Kind::VAR: {
                string type = contact.negated ? "NC" : "NO";
                ss << "        <contact address=\"" << escapeXml(contact.name) << "\" type=\"" << type << "\"/>\n";
                break;
            }
            case Contact::Kind::COMP: {
                ss << "        <contact type=\"comparison\" op=\"" << escapeXml(contact.op)
                   << "\" left=\"" << escapeXml(contact.left) << "\" right=\"" << escapeXml(contact.right) << "\"/>\n";
                break;
            }
            case Contact::Kind::EDGE: {
                ss << "        <contact address=\"" << escapeXml(contact.name)
                   << "\" type=\"" << escapeXml(contact.op) << "\"/>\n";
                break;
            }
        }
    }
}

string generateRung(const vector<Term>& terms, const string& coilVar, const string& coilType = "set") {
    stringstream ss;
    ss << "    <rung>\n";
    if (terms.empty()) {
        ss << "      <coil address=\"" << escapeXml(coilVar) << "\" type=\"reset\"/>\n";
    } else {
        for (const auto& term : terms) {
            ss << "      <branch>\n";
            writeContacts(ss, term.contacts);
            ss << "      </branch>\n";
        }
        if (coilType == "reset") {
            ss << "      <coil address=\"" << escapeXml(coilVar) << "\" type=\"reset\"/>\n";
        } else {
            ss << "      <coil address=\"" << escapeXml(coilVar) << "\"/>\n";
        }
    }
    ss << "    </rung>\n";
    return ss.str();
}

string generateMoveRung(const vector<Term>& terms, const string& dest, const string& source) {
    stringstream ss;
    ss << "    <rung>\n";
    if (terms.empty()) {
        ss << "      <move dest=\"" << escapeXml(dest) << "\" source=\"" << escapeXml(source) << "\"/>\n";
    } else {
        for (const auto& term : terms) {
            ss << "      <branch>\n";
            writeContacts(ss, term.contacts);
            ss << "      </branch>\n";
        }
        ss << "      <move dest=\"" << escapeXml(dest) << "\" source=\"" << escapeXml(source) << "\"/>\n";
    }
    ss << "    </rung>\n";
    return ss.str();
}

string generateLabelRung(const string& label) {
    stringstream ss;
    ss << "    <rung>\n";
    ss << "      <label name=\"" << escapeXml(label) << "\"/>\n";
    ss << "    </rung>\n";
    return ss.str();
}

string generateJumpRung(const string& label, const string& type = "jmp", const vector<Term>* condTerms = nullptr) {
    stringstream ss;
    ss << "    <rung>\n";
    if (condTerms) {
        for (const auto& term : *condTerms) {
            ss << "      <branch>\n";
            writeContacts(ss, term.contacts);
            ss << "      </branch>\n";
        }
    }
    if (type == "jmpn") {
        ss << "      <jump type=\"jmpn\" label=\"" << escapeXml(label) << "\"/>\n";
    } else {
        ss << "      <jump type=\"jmp\" label=\"" << escapeXml(label) << "\"/>\n";
    }
    ss << "    </rung>\n";
    return ss.str();
}

string generateTimerRung(const vector<Term>& terms, const string& timerType, const string& duration, const string& outputVar) {
    stringstream ss;
    ss << "    <rung>\n";
    for (const auto& term : terms) {
        ss << "      <branch>\n";
        writeContacts(ss, term.contacts);
        ss << "      </branch>\n";
    }
    ss << "      <timer type=\"" << escapeXml(timerType) << "\" duration=\"" << escapeXml(duration) << "\" output=\"" << escapeXml(outputVar) << "\"/>\n";
    ss << "    </rung>\n";
    return ss.str();
}

string generateCounterRung(const string& counterType, const vector<string>& args, const string& preset, const string& outputVar) {
    stringstream ss;
    ss << "    <rung>\n";
    ss << "      <counter type=\"" << escapeXml(counterType) << "\"";
    if (counterType == "count_up") {
        ss << " input=\"" << escapeXml(args[0]) << "\" reset=\"" << escapeXml(args[1]) << "\"";
    } else if (counterType == "count_down") {
        ss << " input=\"" << escapeXml(args[0]) << "\" load=\"" << escapeXml(args[1]) << "\"";
    } else if (counterType == "count_updown") {
        ss << " up=\"" << escapeXml(args[0]) << "\" down=\"" << escapeXml(args[1]) << "\" reset=\"" << escapeXml(args[2]) << "\" load=\"" << escapeXml(args[3]) << "\"";
    }
    ss << " preset=\"" << escapeXml(preset) << "\" output=\"" << escapeXml(outputVar) << "\"/>\n";
    ss << "    </rung>\n";
    return ss.str();
}

// کلون کردن عبارت با جایگزینی نام‌ها بر اساس نقشه جانشینی (پارامتر تابع یا متغیر حلقه)
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
    if (auto breakStmt = dynamic_cast<const BreakStmt*>(&stmt)) {
        return make_unique<BreakStmt>(breakStmt->line, breakStmt->column);
    }
    if (auto continueStmt = dynamic_cast<const ContinueStmt*>(&stmt)) {
        return make_unique<ContinueStmt>(continueStmt->line, continueStmt->column);
    }
    if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        ExprPtr cond = cloneExprWithSubst(*ifStmt->cond, subs);
        vector<StmtPtr> thenBlock;
        for (const auto& s : ifStmt->thenBlock) {
            thenBlock.push_back(cloneStmtWithSubst(*s, subs));
        }
        vector<pair<ExprPtr, vector<StmtPtr>>> elifBranches;
        for (const auto& branch : ifStmt->elifBranches) {
            ExprPtr elifCond = cloneExprWithSubst(*branch.first, subs);
            vector<StmtPtr> elifBlock;
            for (const auto& s : branch.second) {
                elifBlock.push_back(cloneStmtWithSubst(*s, subs));
            }
            elifBranches.emplace_back(std::move(elifCond), std::move(elifBlock));
        }
        vector<StmtPtr> elseBlock;
        for (const auto& s : ifStmt->elseBlock) {
            elseBlock.push_back(cloneStmtWithSubst(*s, subs));
        }
        return make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elifBranches), std::move(elseBlock), ifStmt->line, ifStmt->column);
    }
    if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        vector<StmtPtr> body;
        for (const auto& s : forStmt->body) {
            body.push_back(cloneStmtWithSubst(*s, subs));
        }
        return make_unique<ForStmt>(forStmt->varName, forStmt->start, forStmt->end, std::move(body), forStmt->line, forStmt->column);
    }
    if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        ExprPtr cond = cloneExprWithSubst(*whileStmt->cond, subs);
        vector<StmtPtr> body;
        for (const auto& s : whileStmt->body) {
            body.push_back(cloneStmtWithSubst(*s, subs));
        }
        return make_unique<WhileStmt>(std::move(cond), std::move(body), whileStmt->line, whileStmt->column);
    }
    return nullptr;
}

} // namespace

string generateLadderXml(const Program& program, const Config& config) {
    stringstream out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<networks>\n";

    int networkCounter = 0;
    int whileCounter = 0;
    int callDepth = 0;
    const int kMaxInlineDepth = 8;

    // استک برچسب‌های حلقه برای break/continue
    vector<string> breakLabels;
    vector<string> continueLabels;

    // جدول توابع کاربر (هر ترتیب تعریفی پذیرفته است)
    unordered_map<string, const FunctionDef*> funcMap;
    for (const auto& func : program.functions) {
        if (!funcMap.count(func->name)) {
            funcMap[func->name] = func.get();
        }
    }

    std::function<void(const Stmt&)> processStmt;

    // درج بدنه تابع کاربر در محل فراخوانی با جایگزینی پارامترها
    std::function<void(const CallStmt&, const SubstMap&)> expandCall =
        [&](const CallStmt& call, const SubstMap& outerSubs) {
            auto it = funcMap.find(builtins::normalize(call.funcName));
            if (it == funcMap.end()) return;  // semantic قبلاً خطا داده
            const FunctionDef* func = it->second;

            if (callDepth >= kMaxInlineDepth) {
                throw runtime_error("Inline expansion too deep at line " +
                                    to_string(call.line) +
                                    " (recursive function call?)");
            }

            // کلون آرگومان‌ها نسبت به جانشینی بیرونی و ساخت نقشه پارامترها
            SubstMap subs(outerSubs);
            vector<ExprPtr> clonedArgs;
            size_t n = min(func->params.size(), call.args.size());
            for (size_t i = 0; i < n; ++i) {
                clonedArgs.push_back(cloneExprWithSubst(*call.args[i], outerSubs));
                subs[func->params[i]] = clonedArgs.back().get();
            }

            callDepth++;
            for (const auto& stmt : func->body) {
                StmtPtr cloned = cloneStmtWithSubst(*stmt, subs);
                if (cloned) processStmt(*cloned);
            }
            callDepth--;
        };

    processStmt = [&](const Stmt& stmt) {
        if (auto assign = dynamic_cast<const AssignmentStmt*>(&stmt)) {
            auto it = config.io.find(assign->name);
            if (it != config.io.end()) {
                string varType = it->second.type;
                if (varType == "BOOL") {
                    // Timer handling
                    if (auto call = dynamic_cast<CallExpr*>(assign->expr.get())) {
                        const string canonical = builtins::normalize(call->funcName);
                        if (builtins::isTimer(canonical)) {
                            auto terms = exprToDNF(*call->args[0], config);
                            string duration = exprToString(*call->args[1], &config);
                            out << "  <network name=\"net" << ++networkCounter << "\">\n";
                            out << generateTimerRung(terms, canonical, duration, assign->name);
                            out << "  </network>\n";
                            return;
                        }
                        else if (builtins::isCounter(canonical)) {
                            vector<string> argStrs;
                            for (const auto& arg : call->args) {
                                argStrs.push_back(exprToString(*arg, &config));
                            }
                            string preset = argStrs.back();
                            argStrs.pop_back(); // حذف preset
                            out << "  <network name=\"net" << ++networkCounter << "\">\n";
                            out << generateCounterRung(canonical, argStrs, preset, assign->name);
                            out << "  </network>\n";
                            return;
                        }
                    }
                    // Normal BOOL assignment
                    if (auto boolExpr = dynamic_cast<const BoolExpr*>(assign->expr.get())) {
                        if (!boolExpr->value) {
                            out << "  <network name=\"net" << ++networkCounter << "\">\n";
                            out << "    <rung>\n";
                            out << "      <coil address=\"" << escapeXml(assign->name) << "\" type=\"reset\"/>\n";
                            out << "    </rung>\n";
                            out << "  </network>\n";
                            return;
                        }
                    }
                    auto terms = exprToDNF(*assign->expr, config);
                    out << "  <network name=\"net" << ++networkCounter << "\">\n";
                    out << generateRung(terms, assign->name, "set");
                    out << "  </network>\n";
                } else {
                    string source = exprToString(*assign->expr, &config);
                    out << "  <network name=\"net" << ++networkCounter << "\">\n";
                    out << generateMoveRung({}, assign->name, source);
                    out << "  </network>\n";
                }
            }
        }
        else if (auto idxAssign = dynamic_cast<const IndexAssignmentStmt*>(&stmt)) {
            auto it = config.io.find(idxAssign->name);
            if (it != config.io.end()) {
                string varType = it->second.type;
                string indexStr = exprToString(*idxAssign->index, &config);
                string addr = resolveArrayAddress(it->second.address, varType, indexStr);

                if (varType == "BOOL") {
                    if (auto boolExpr = dynamic_cast<const BoolExpr*>(idxAssign->expr.get())) {
                        if (!boolExpr->value) {
                            out << "  <network name=\"net" << ++networkCounter << "\">\n";
                            out << "    <rung>\n";
                            out << "      <coil address=\"" << escapeXml(addr) << "\" type=\"reset\"/>\n";
                            out << "    </rung>\n";
                            out << "  </network>\n";
                            return;
                        }
                    }
                    auto terms = exprToDNF(*idxAssign->expr, config);
                    out << "  <network name=\"net" << ++networkCounter << "\">\n";
                    out << generateRung(terms, addr, "set");
                    out << "  </network>\n";
                } else {
                    string source = exprToString(*idxAssign->expr, &config);
                    out << "  <network name=\"net" << ++networkCounter << "\">\n";
                    out << generateMoveRung({}, addr, source);
                    out << "  </network>\n";
                }
            }
        }
        else if (auto callStmt = dynamic_cast<const CallStmt*>(&stmt)) {
            SubstMap emptySubs;
            expandCall(*callStmt, emptySubs);
        }
        else if (auto breakStmt = dynamic_cast<const BreakStmt*>(&stmt)) {
            if (!breakLabels.empty()) {
                out << "  <network name=\"net" << ++networkCounter << "\">\n";
                out << generateJumpRung(breakLabels.back(), "jmp");
                out << "  </network>\n";
            }
        }
        else if (auto continueStmt = dynamic_cast<const ContinueStmt*>(&stmt)) {
            if (!continueLabels.empty()) {
                out << "  <network name=\"net" << ++networkCounter << "\">\n";
                out << generateJumpRung(continueLabels.back(), "jmp");
                out << "  </network>\n";
            }
        }
        else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
            struct BranchInfo {
                vector<Term> cond;
                const vector<StmtPtr>* statements;
            };

            vector<BranchInfo> branches;

            auto cond = exprToDNF(*ifStmt->cond, config);
            branches.push_back({cond, &ifStmt->thenBlock});

            vector<Term> negatedSoFar = cond;
            for (const auto& branch : ifStmt->elifBranches) {
                auto elifCond = exprToDNF(*branch.first, config);
                auto notPrev = negateDNF(negatedSoFar);
                vector<Term> combined;
                for (auto& np : notPrev) {
                    for (auto& ec : elifCond) {
                        Term t = np;
                        t.contacts.insert(t.contacts.end(), ec.contacts.begin(), ec.contacts.end());
                        combined.push_back(move(t));
                    }
                }
                branches.push_back({combined, &branch.second});
                negatedSoFar.insert(negatedSoFar.end(), elifCond.begin(), elifCond.end());
            }

            if (!ifStmt->elseBlock.empty()) {
                auto elseCond = negateDNF(negatedSoFar);
                branches.push_back({elseCond, &ifStmt->elseBlock});
            }

            for (const auto& branch : branches) {
                const auto& branchCond = branch.cond;
                const auto& statements = *branch.statements;
                for (const auto& innerStmt : statements) {
                    if (auto innerAssign = dynamic_cast<const AssignmentStmt*>(innerStmt.get())) {
                        auto it = config.io.find(innerAssign->name);
                        if (it != config.io.end()) {
                            string varType = it->second.type;
                            if (varType == "BOOL") {
                                bool isReset = false;
                                if (auto boolExpr = dynamic_cast<const BoolExpr*>(innerAssign->expr.get())) {
                                    if (!boolExpr->value) isReset = true;
                                }
                                if (!isReset) {
                                    auto assignCond = exprToDNF(*innerAssign->expr, config);
                                    vector<Term> combined;
                                    for (auto& bc : branchCond) {
                                        for (auto& ac : assignCond) {
                                            Term t = bc;
                                            t.contacts.insert(t.contacts.end(), ac.contacts.begin(), ac.contacts.end());
                                            combined.push_back(move(t));
                                        }
                                    }
                                    if (!combined.empty()) {
                                        out << "  <network name=\"net" << ++networkCounter << "\">\n";
                                        out << generateRung(combined, innerAssign->name, "set");
                                        out << "  </network>\n";
                                    }
                                } else {
                                    out << "  <network name=\"net" << ++networkCounter << "\">\n";
                                    out << generateRung(branchCond, innerAssign->name, "reset");
                                    out << "  </network>\n";
                                }
                            } else {
                                string source = exprToString(*innerAssign->expr, &config);
                                out << "  <network name=\"net" << ++networkCounter << "\">\n";
                                out << generateMoveRung(branchCond, innerAssign->name, source);
                                out << "  </network>\n";
                            }
                        }
                    }
                    else if (auto innerIdxAssign = dynamic_cast<const IndexAssignmentStmt*>(innerStmt.get())) {
                        auto it = config.io.find(innerIdxAssign->name);
                        if (it != config.io.end()) {
                            string varType = it->second.type;
                            string indexStr = exprToString(*innerIdxAssign->index, &config);
                            string addr = resolveArrayAddress(it->second.address, varType, indexStr);
                            if (varType == "BOOL") {
                                bool isReset = false;
                                if (auto boolExpr = dynamic_cast<const BoolExpr*>(innerIdxAssign->expr.get())) {
                                    if (!boolExpr->value) isReset = true;
                                }
                                if (!isReset) {
                                    auto assignCond = exprToDNF(*innerIdxAssign->expr, config);
                                    vector<Term> combined;
                                    for (auto& bc : branchCond) {
                                        for (auto& ac : assignCond) {
                                            Term t = bc;
                                            t.contacts.insert(t.contacts.end(), ac.contacts.begin(), ac.contacts.end());
                                            combined.push_back(move(t));
                                        }
                                    }
                                    if (!combined.empty()) {
                                        out << "  <network name=\"net" << ++networkCounter << "\">\n";
                                        out << generateRung(combined, addr, "set");
                                        out << "  </network>\n";
                                    }
                                } else {
                                    out << "  <network name=\"net" << ++networkCounter << "\">\n";
                                    out << generateRung(branchCond, addr, "reset");
                                    out << "  </network>\n";
                                }
                            } else {
                                string source = exprToString(*innerIdxAssign->expr, &config);
                                out << "  <network name=\"net" << ++networkCounter << "\">\n";
                                out << generateMoveRung(branchCond, addr, source);
                                out << "  </network>\n";
                            }
                        }
                    }
                    // break/continue شرطی داخل شاخه if:
                    // jmpn وقتی پرش می‌کند که جریان برق قطع باشد؛ پس شرطِ نقیض‌شده شاخه را
                    // می‌فرستیم تا پرش دقیقاً وقتی رخ دهد که شاخه برقرار است
                    else if (auto brk = dynamic_cast<const BreakStmt*>(innerStmt.get())) {
                        if (!breakLabels.empty()) {
                            auto negated = negateDNF(branchCond);
                            out << "  <network name=\"net" << ++networkCounter << "\">\n";
                            out << generateJumpRung(breakLabels.back(), "jmpn", &negated);
                            out << "  </network>\n";
                        }
                    }
                    else if (auto cont = dynamic_cast<const ContinueStmt*>(innerStmt.get())) {
                        if (!continueLabels.empty()) {
                            auto negated = negateDNF(branchCond);
                            out << "  <network name=\"net" << ++networkCounter << "\">\n";
                            out << generateJumpRung(continueLabels.back(), "jmpn", &negated);
                            out << "  </network>\n";
                        }
                    }
                    // سایر انواع دستور مرکب داخل شاخه if در semantic_analyzer رد می‌شوند
                }
            }
        }
        else if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
            for (int i = forStmt->start; i < forStmt->end; ++i) {
                // نگاشت متغیر حلقه به مقدار فعلی
                NumberExpr loopValue(to_string(i), false, forStmt->line, forStmt->column);
                SubstMap subs;
                subs[forStmt->varName] = &loopValue;
                for (const auto& innerStmt : forStmt->body) {
                    StmtPtr cloned = cloneStmtWithSubst(*innerStmt, subs);
                    if (cloned) {
                        processStmt(*cloned);
                    }
                }
            }
        }
        else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
            whileCounter++;
            string startLabel = "WHILE_START_" + to_string(whileCounter);
            string endLabel = "WHILE_END_" + to_string(whileCounter);

            out << "  <network name=\"net" << ++networkCounter << "\">\n";
            out << generateLabelRung(startLabel);
            out << "  </network>\n";

            auto condTerms = exprToDNF(*whileStmt->cond, config);
            out << "  <network name=\"net" << ++networkCounter << "\">\n";
            out << generateJumpRung(endLabel, "jmpn", &condTerms);
            out << "  </network>\n";

            breakLabels.push_back(endLabel);
            continueLabels.push_back(startLabel);
            for (const auto& innerStmt : whileStmt->body) {
                processStmt(*innerStmt);
            }
            breakLabels.pop_back();
            continueLabels.pop_back();

            out << "  <network name=\"net" << ++networkCounter << "\">\n";
            out << generateJumpRung(startLabel, "jmp");
            out << "  </network>\n";

            out << "  <network name=\"net" << ++networkCounter << "\">\n";
            out << generateLabelRung(endLabel);
            out << "  </network>\n";
        }
    };

    for (const auto& func : program.functions) {
        if (func->name != "main") continue;
        for (const auto& stmt : func->body) {
            processStmt(*stmt);
        }
    }

    out << "</networks>\n";
    return out.str();
}
