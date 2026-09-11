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

// String literal: "text"
struct StringExpr : Expr {
    std::string value;
    StringExpr(std::string v, int ln, int col) : Expr(ln, col), value(std::move(v)) {}
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(std::string n, int ln, int col) : Expr(ln, col), name(std::move(n)) {}
};
// Function call (e.g. on_delay(...))
struct CallExpr : Expr {
    std::string funcName;
    std::vector<ExprPtr> args;
    CallExpr(std::string name, std::vector<ExprPtr> a, int ln, int col)
        : Expr(ln, col), funcName(std::move(name)), args(std::move(a)) {}
};

// Member access (optional, currently unused but reserved)
struct AttributeExpr : Expr {
    std::string objectName;
    std::string attrName;
    AttributeExpr(std::string obj, std::string attr, int ln, int col)
        : Expr(ln, col), objectName(std::move(obj)), attrName(std::move(attr)) {}
};

// Dot access on struct: object.field
struct FieldAccessExpr : Expr {
    ExprPtr object;
    std::string field;
    FieldAccessExpr(ExprPtr obj, std::string f, int ln, int col)
        : Expr(ln, col), object(std::move(obj)), field(std::move(f)) {}
};

// Struct literal: StructName { field1: expr1, field2: expr2 }
struct StructLiteralExpr : Expr {
    std::string structName;
    std::vector<std::pair<std::string, ExprPtr>> fields;
    StructLiteralExpr(std::string n, std::vector<std::pair<std::string, ExprPtr>> f, int ln, int col)
        : Expr(ln, col), structName(std::move(n)), fields(std::move(f)) {}
};

// Match expression: match expr { case pattern: result, ... }
struct MatchCase {
    std::string pattern;          // enum variant name or literal
    std::vector<std::string> vars; // extracted variables (for tuple variants)
    ExprPtr result;
    MatchCase(std::string p, std::vector<std::string> v, ExprPtr r)
        : pattern(std::move(p)), vars(std::move(v)), result(std::move(r)) {}
};

struct MatchExpr : Expr {
    ExprPtr scrutinee;
    std::vector<MatchCase> cases;
    MatchExpr(ExprPtr s, std::vector<MatchCase> c, int ln, int col)
        : Expr(ln, col), scrutinee(std::move(s)), cases(std::move(c)) {}
};
struct IndexExpr : Expr {
    std::string name;      // array name
    ExprPtr index;         // index expression (typically a number or loop variable)
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

// Python-style conditional expression: trueExpr if cond else falseExpr
struct TernaryExpr : Expr {
    ExprPtr cond;
    ExprPtr trueExpr;
    ExprPtr falseExpr;
    TernaryExpr(ExprPtr c, ExprPtr t, ExprPtr f, int ln, int col)
        : Expr(ln, col), cond(std::move(c)), trueExpr(std::move(t)), falseExpr(std::move(f)) {}
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

// Struct field assignment: object.field = expr
struct FieldAssignmentStmt : Stmt {
    ExprPtr object;
    std::string field;
    ExprPtr expr;
    FieldAssignmentStmt(ExprPtr obj, std::string f, ExprPtr e, int ln, int col)
        : Stmt(ln, col), object(std::move(obj)), field(std::move(f)), expr(std::move(e)) {}
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

// Break out of a loop (only valid inside while)
struct BreakStmt : Stmt {
    BreakStmt(int ln, int col) : Stmt(ln, col) {}
};

// Jump to the start of a loop (only valid inside while)
struct ContinueStmt : Stmt {
    ContinueStmt(int ln, int col) : Stmt(ln, col) {}
};

// Function return value: return [expr]
struct ReturnStmt : Stmt {
    ExprPtr value;   // may be null (bare return)
    bool hasValue;
    ReturnStmt(ExprPtr v, bool hv, int ln, int col)
        : Stmt(ln, col), value(std::move(v)), hasValue(hv) {}
};

// User function call as a standalone statement (no return value; effect via global variables)
struct CallStmt : Stmt {
    std::string funcName;
    std::vector<ExprPtr> args;
    CallStmt(std::string name, std::vector<ExprPtr> a, int ln, int col)
        : Stmt(ln, col), funcName(std::move(name)), args(std::move(a)) {}
};

// Match statement: match expr { case pattern: body, ... }
struct MatchStmtCase {
    std::string pattern;
    std::vector<std::string> vars;
    std::vector<StmtPtr> body;
    MatchStmtCase(std::string p, std::vector<std::string> v, std::vector<StmtPtr> b)
        : pattern(std::move(p)), vars(std::move(v)), body(std::move(b)) {}
};

struct MatchStmt : Stmt {
    ExprPtr scrutinee;
    std::vector<MatchStmtCase> cases;
    MatchStmt(ExprPtr s, std::vector<MatchStmtCase> c, int ln, int col)
        : Stmt(ln, col), scrutinee(std::move(s)), cases(std::move(c)) {}
};

// try/except/finally statement (error handling)
struct ExceptClause {
    std::string typeName;   // exception type; "*" for catch-all
    std::vector<StmtPtr> body;
    ExceptClause(std::string t, std::vector<StmtPtr> b)
        : typeName(std::move(t)), body(std::move(b)) {}
};

struct TryStmt : Stmt {
    std::vector<StmtPtr> tryBlock;
    std::vector<ExceptClause> handlers;
    std::vector<StmtPtr> finallyBlock;
    TryStmt(std::vector<StmtPtr> tb, std::vector<ExceptClause> h,
            std::vector<StmtPtr> fb, int ln, int col)
        : Stmt(ln, col), tryBlock(std::move(tb)), handlers(std::move(h)),
          finallyBlock(std::move(fb)) {}
};

struct RaiseStmt : Stmt {
    std::string typeName;   // exception type name
    ExprPtr message;        // optional error message expression
    RaiseStmt(std::string t, ExprPtr m, int ln, int col)
        : Stmt(ln, col), typeName(std::move(t)), message(std::move(m)) {}
};

// ---------------- Struct & Enum Definitions ----------------

struct FieldDef {
    std::string name;
    std::string typeName;  // BOOL, INT, REAL, TIME, or struct name
    FieldDef(std::string n, std::string t) : name(std::move(n)), typeName(std::move(t)) {}
};

struct StructDef {
    std::string name;
    std::vector<FieldDef> fields;
    int line;
    int column;
    StructDef(std::string n, std::vector<FieldDef> f, int ln, int col)
        : name(std::move(n)), fields(std::move(f)), line(ln), column(col) {}
};

struct EnumVariant {
    std::string name;
    std::vector<std::string> associatedTypes;  // for tuple variants: Variant(INT, BOOL)
    EnumVariant(std::string n, std::vector<std::string> t = {})
        : name(std::move(n)), associatedTypes(std::move(t)) {}
};

struct EnumDef {
    std::string name;
    std::vector<EnumVariant> variants;
    int line;
    int column;
    EnumDef(std::string n, std::vector<EnumVariant> v, int ln, int col)
        : name(std::move(n)), variants(std::move(v)), line(ln), column(col) {}
};

// ---------------- Functions & Program ----------------
struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    int line;
    int column;
    // Module/namespace the function belongs to ("" for the main module)
    std::string module;

    FunctionDef(std::string n, std::vector<std::string> p, std::vector<StmtPtr> b, int ln, int col)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)), line(ln), column(col) {}
};

// Module import: import "path.q"  |  import "module_name"
struct ImportStmt {
    std::string path;       // file path or module name
    std::string alias;      // optional: import "foo.q" as foo
    int line;
    int column;
    ImportStmt(std::string p, std::string a, int ln, int col)
        : path(std::move(p)), alias(std::move(a)), line(ln), column(col) {}
};

struct Program {
    std::vector<std::unique_ptr<FunctionDef>> functions;
    std::vector<std::unique_ptr<StructDef>> structs;
    std::vector<std::unique_ptr<EnumDef>> enums;
    std::vector<ImportStmt> imports;
};