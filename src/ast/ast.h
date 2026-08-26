#pragma once

#include <memory>
#include <string>
#include <vector>

// Node base with source position
struct Node {
    int line;
    int column;
    Node(int ln = 0, int col = 0) : line(ln), column(col) {}
    virtual ~Node() = default;
};

// ---------------- Expressions ----------------
struct Expr : Node {
    Expr(int ln, int col) : Node(ln, col) {}
};
using ExprPtr = std::unique_ptr<Expr>;

struct NumberExpr : Expr {
    std::string value;
    bool isFloat;
    NumberExpr(std::string v, bool f, int ln, int col)
        : Expr(ln, col), value(std::move(v)), isFloat(f) {}
};

struct BoolExpr : Expr {
    bool value;
    BoolExpr(bool v, int ln, int col) : Expr(ln, col), value(v) {}
};

struct TimeExpr : Expr {
    std::string value;
    TimeExpr(std::string v, int ln, int col) : Expr(ln, col), value(std::move(v)) {}
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(std::string n, int ln, int col) : Expr(ln, col), name(std::move(n)) {}
};
// فراخوانی تابع (مثل on_delay(...))
struct CallExpr : Expr {
    std::string funcName;
    std::vector<ExprPtr> args;
    CallExpr(std::string name, std::vector<ExprPtr> a, int ln, int col)
        : Expr(ln, col), funcName(std::move(name)), args(std::move(a)) {}
};

// دسترسی به عضو (اختیاری، فعلاً استفاده نمی‌شود ولی آماده است)
struct AttributeExpr : Expr {
    std::string objectName;
    std::string attrName;
    AttributeExpr(std::string obj, std::string attr, int ln, int col)
        : Expr(ln, col), objectName(std::move(obj)), attrName(std::move(attr)) {}
};
struct IndexExpr : Expr {
    std::string name;      // نام آرایه
    ExprPtr index;         // عبارت اندیس (معمولاً عدد یا متغیر حلقه)
    IndexExpr(std::string n, ExprPtr i, int ln, int col)
        : Expr(ln, col), name(std::move(n)), index(std::move(i)) {}
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr operand;
    UnaryExpr(std::string o, ExprPtr e, int ln, int col)
        : Expr(ln, col), op(std::move(o)), operand(std::move(e)) {}
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left, right;
    BinaryExpr(std::string o, ExprPtr l, ExprPtr r, int ln, int col)
        : Expr(ln, col), op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};

// ---------------- Statements ----------------
struct Stmt : Node {
    Stmt(int ln, int col) : Node(ln, col) {}
};
using StmtPtr = std::unique_ptr<Stmt>;

struct AssignmentStmt : Stmt {
    std::string name;
    ExprPtr expr;
    AssignmentStmt(std::string n, ExprPtr e, int ln, int col)
        : Stmt(ln, col), name(std::move(n)), expr(std::move(e)) {}
};

struct IndexAssignmentStmt : Stmt {
    std::string name;
    ExprPtr index;
    ExprPtr expr;
    IndexAssignmentStmt(std::string n, ExprPtr i, ExprPtr e, int ln, int col)
        : Stmt(ln, col), name(std::move(n)), index(std::move(i)), expr(std::move(e)) {}
};

struct IfStmt : Stmt {
    ExprPtr cond;
    std::vector<StmtPtr> thenBlock;
    std::vector<std::pair<ExprPtr, std::vector<StmtPtr>>> elifBranches;
    std::vector<StmtPtr> elseBlock;

    IfStmt(ExprPtr c,
           std::vector<StmtPtr> tb,
           std::vector<std::pair<ExprPtr, std::vector<StmtPtr>>> eb,
           std::vector<StmtPtr> el,
           int ln, int col)
        : Stmt(ln, col),
          cond(std::move(c)),
          thenBlock(std::move(tb)),
          elifBranches(std::move(eb)),
          elseBlock(std::move(el)) {}
};

struct ForStmt : Stmt {
    std::string varName;
    int start;
    int end;
    std::vector<StmtPtr> body;

    ForStmt(std::string var, int s, int e, std::vector<StmtPtr> b, int ln, int col)
        : Stmt(ln, col), varName(std::move(var)), start(s), end(e), body(std::move(b)) {}
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    std::vector<StmtPtr> body;

    WhileStmt(ExprPtr c, std::vector<StmtPtr> b, int ln, int col)
        : Stmt(ln, col), cond(std::move(c)), body(std::move(b)) {}
};

// خروج از حلقه (فقط داخل while معتبر است)
struct BreakStmt : Stmt {
    BreakStmt(int ln, int col) : Stmt(ln, col) {}
};

// پرش به ابتدای حلقه (فقط داخل while معتبر است)
struct ContinueStmt : Stmt {
    ContinueStmt(int ln, int col) : Stmt(ln, col) {}
};

// فراخوانی تابع کاربر به‌عنوان دستور مستقل (بدون مقدار بازگشتی؛ اثر از طریق متغیرهای سراسری)
struct CallStmt : Stmt {
    std::string funcName;
    std::vector<ExprPtr> args;
    CallStmt(std::string name, std::vector<ExprPtr> a, int ln, int col)
        : Stmt(ln, col), funcName(std::move(name)), args(std::move(a)) {}
};

// ---------------- Functions & Program ----------------
struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    int line;
    int column;

    FunctionDef(std::string n, std::vector<std::string> p, std::vector<StmtPtr> b, int ln, int col)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)), line(ln), column(col) {}
};

struct Program {
    std::vector<std::unique_ptr<FunctionDef>> functions;
};