#include "parser/parser.h"

#include <stdexcept>

using namespace std;

namespace {

// Reports a parse error at the token's position (main.cpp catches this exception)
[[noreturn]] void parseError(const Token& tok, const string& msg) {
    throw runtime_error("Parse error at line " + to_string(tok.line) +
                        ", col " + to_string(tok.column) + ": " + msg);
}

bool isComparisonOp(const Token& tok) {
    return tok.type == TokenType::OPERATOR &&
           (tok.lexeme == "==" || tok.lexeme == "!=" || tok.lexeme == "<" ||
            tok.lexeme == ">" || tok.lexeme == "<=" || tok.lexeme == ">=");
}

}  // namespace

Parser::Parser(const vector<Token>& tokens) : tokens(tokens) {}

const Token& Parser::current() const {
    return tokens[pos];
}

const Token& Parser::peek(size_t offset) const {
    size_t idx = pos + offset;
    if (idx >= tokens.size()) idx = tokens.size() - 1;  // END_OF_FILE
    return tokens[idx];
}

bool Parser::check(TokenType type, const string& lexeme) const {
    if (current().type != type) return false;
    if (!lexeme.empty() && current().lexeme != lexeme) return false;
    return true;
}

const Token& Parser::advance() {
    const Token& tok = current();
    if (tok.type != TokenType::END_OF_FILE) pos++;
    return tok;
}

const Token& Parser::expect(TokenType type, const string& lexeme) {
    if (!check(type, lexeme)) {
        string expected = lexeme.empty() ? tokenTypeToString(type) : "'" + lexeme + "'";
        parseError(current(), "expected " + expected + " but found '" + current().lexeme + "'");
    }
    return advance();
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE)) {
        pos++;
    }
}

// ---------------- Program & Functions ----------------

unique_ptr<Program> Parser::parseProgram() {
    auto program = make_unique<Program>();
    skipNewlines();
    while (!check(TokenType::END_OF_FILE)) {
        if (!check(TokenType::KEYWORD, "def")) {
            parseError(current(), "expected 'def' at top level, found '" + current().lexeme + "'");
        }
        program->functions.push_back(parseFunctionDef());
        skipNewlines();
    }
    return program;
}

unique_ptr<FunctionDef> Parser::parseFunctionDef() {
    const Token& defTok = expect(TokenType::KEYWORD, "def");
    string name = expect(TokenType::IDENTIFIER).lexeme;

    expect(TokenType::PUNCTUATION, "(");
    vector<string> params;
    if (check(TokenType::IDENTIFIER)) {
        params.push_back(advance().lexeme);
        while (check(TokenType::PUNCTUATION, ",")) {
            advance();
            params.push_back(expect(TokenType::IDENTIFIER).lexeme);
        }
    }
    expect(TokenType::PUNCTUATION, ")");
    expect(TokenType::PUNCTUATION, ":");

    auto body = parseBlock();
    return make_unique<FunctionDef>(name, move(params), move(body), defTok.line, defTok.column);
}

// ---------------- Blocks & Statements ----------------

vector<StmtPtr> Parser::parseBlock() {
    expect(TokenType::NEWLINE);
    if (!check(TokenType::INDENT)) {
        parseError(current(), "expected an indented block");
    }
    advance();

    vector<StmtPtr> stmts = parseStatements();

    if (!check(TokenType::DEDENT)) {
        parseError(current(), "expected dedent at end of block, found '" + current().lexeme + "'");
    }
    advance();
    return stmts;
}

vector<StmtPtr> Parser::parseStatements() {
    vector<StmtPtr> stmts;
    while (true) {
        skipNewlines();
        if (check(TokenType::DEDENT) || check(TokenType::END_OF_FILE)) break;
        stmts.push_back(parseStatement());
    }
    return stmts;
}

StmtPtr Parser::parseStatement() {
    if (check(TokenType::KEYWORD, "if")) return parseIfStmt();
    if (check(TokenType::KEYWORD, "while")) return parseWhileStmt();
    if (check(TokenType::KEYWORD, "for")) return parseForStmt();
    if (check(TokenType::KEYWORD, "break") || check(TokenType::KEYWORD, "continue")) {
        const Token& kwTok = advance();
        expect(TokenType::NEWLINE);
        if (kwTok.lexeme == "break") {
            return make_unique<BreakStmt>(kwTok.line, kwTok.column);
        }
        return make_unique<ContinueStmt>(kwTok.line, kwTok.column);
    }
    // Function call as a statement: name(args)
    if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::PUNCTUATION &&
        peek(1).lexeme == "(") {
        return parseCallStmt();
    }
    if (check(TokenType::IDENTIFIER)) return parseAssignment();
    if (check(TokenType::KEYWORD, "return")) return parseReturnStmt();
    parseError(current(), "expected a statement, found '" + current().lexeme + "'");
}

// name(arg1, arg2, ...) as a standalone statement
StmtPtr Parser::parseCallStmt() {
    const Token& nameTok = expect(TokenType::IDENTIFIER);
    string name = nameTok.lexeme;

    expect(TokenType::PUNCTUATION, "(");
    vector<ExprPtr> args;
    if (!check(TokenType::PUNCTUATION, ")")) {
        args.push_back(parseExpression());
        while (check(TokenType::PUNCTUATION, ",")) {
            advance();
            args.push_back(parseExpression());
        }
    }
    expect(TokenType::PUNCTUATION, ")");
    expect(TokenType::NEWLINE);
    return make_unique<CallStmt>(name, move(args), nameTok.line, nameTok.column);
}

StmtPtr Parser::parseReturnStmt() {
    const Token& returnTok = expect(TokenType::KEYWORD, "return");
    ExprPtr value = nullptr;
    bool hasValue = false;

    // return [expr] - if not a NEWLINE, parse the expression
    if (!check(TokenType::NEWLINE)) {
        value = parseExpression();
        hasValue = true;
    }
    expect(TokenType::NEWLINE);
    return make_unique<ReturnStmt>(move(value), hasValue, returnTok.line, returnTok.column);
}

unique_ptr<IfStmt> Parser::parseIfStmt() {
    const Token& ifTok = expect(TokenType::KEYWORD, "if");
    auto cond = parseExpression();
    expect(TokenType::PUNCTUATION, ":");
    auto thenBlock = parseBlock();

    vector<pair<ExprPtr, vector<StmtPtr>>> elifBranches;
    while (check(TokenType::KEYWORD, "elif")) {
        skipNewlines();  // Guard against blank lines between branches (lexer removes them)
        advance();
        auto elifCond = parseExpression();
        expect(TokenType::PUNCTUATION, ":");
        auto elifBody = parseBlock();
        elifBranches.emplace_back(move(elifCond), move(elifBody));
    }

    vector<StmtPtr> elseBlock;
    if (check(TokenType::KEYWORD, "else")) {
        skipNewlines();
        advance();
        expect(TokenType::PUNCTUATION, ":");
        elseBlock = parseBlock();
    }

    return make_unique<IfStmt>(move(cond), move(thenBlock), move(elifBranches),
                               move(elseBlock), ifTok.line, ifTok.column);
}

unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    const Token& whileTok = expect(TokenType::KEYWORD, "while");
    auto cond = parseExpression();
    expect(TokenType::PUNCTUATION, ":");
    auto body = parseBlock();
    return make_unique<WhileStmt>(move(cond), move(body), whileTok.line, whileTok.column);
}

unique_ptr<ForStmt> Parser::parseForStmt() {
    const Token& forTok = expect(TokenType::KEYWORD, "for");
    string varName = expect(TokenType::IDENTIFIER).lexeme;
    expect(TokenType::KEYWORD, "in");
    expect(TokenType::KEYWORD, "range");
    expect(TokenType::PUNCTUATION, "(");

    auto readInt = [&]() -> int {
        bool negative = false;
        if (check(TokenType::OPERATOR, "-")) {
            negative = true;
            advance();
        }
        const Token& numTok = expect(TokenType::INTEGER);
        int value = stoi(numTok.lexeme);
        return negative ? -value : value;
    };

    int first = readInt();
    int start = 0;
    int end = first;
    if (check(TokenType::PUNCTUATION, ",")) {
        advance();
        end = readInt();
        start = first;
    }
    // Third range argument (step) is not supported
    if (check(TokenType::PUNCTUATION, ",")) {
        parseError(current(), "range() with a step argument is not supported");
    }

    expect(TokenType::PUNCTUATION, ")");
    expect(TokenType::PUNCTUATION, ":");
    auto body = parseBlock();
    return make_unique<ForStmt>(varName, start, end, move(body), forTok.line, forTok.column);
}

StmtPtr Parser::parseAssignment() {
    const Token& nameTok = expect(TokenType::IDENTIFIER);
    string name = nameTok.lexeme;

    // Indexed assignment: name[index] = expr
    if (check(TokenType::PUNCTUATION, "[")) {
        advance();
        auto index = parseExpression();
        expect(TokenType::PUNCTUATION, "]");
        expect(TokenType::OPERATOR, "=");
        auto expr = parseExpression();
        return make_unique<IndexAssignmentStmt>(name, move(index), move(expr),
                                                nameTok.line, nameTok.column);
    }

    expect(TokenType::OPERATOR, "=");
    auto expr = parseExpression();
    // Simple statement is always terminated by NEWLINE (lexer emits NEWLINE after each content line)
    expect(TokenType::NEWLINE);
    return make_unique<AssignmentStmt>(name, move(expr), nameTok.line, nameTok.column);
}

// ---------------- Expressions (precedence climbing) ----------------

ExprPtr Parser::parseExpression() {
    return parseTernary();
}

ExprPtr Parser::parseTernary() {
    auto left = parseOr();

    // Python-style ternary: true_expr if cond else false_expr
    if (check(TokenType::KEYWORD, "if")) {
        const Token& ifTok = advance();
        auto cond = parseOr();
        expect(TokenType::KEYWORD, "else");
        auto falseExpr = parseOr();
        return make_unique<TernaryExpr>(move(cond), move(left), move(falseExpr), ifTok.line, ifTok.column);
    }
    return left;
}

ExprPtr Parser::parseOr() {
    auto left = parseAnd();
    while (check(TokenType::KEYWORD, "or") || check(TokenType::KEYWORD, "xor")) {
        const Token& opTok = advance();
        auto right = parseAnd();
        left = make_unique<BinaryExpr>(opTok.lexeme, move(left), move(right), opTok.line, opTok.column);
    }
    return left;
}

ExprPtr Parser::parseAnd() {
    auto left = parseNot();
    while (check(TokenType::KEYWORD, "and")) {
        const Token& opTok = advance();
        auto right = parseNot();
        left = make_unique<BinaryExpr>("and", move(left), move(right), opTok.line, opTok.column);
    }
    return left;
}

ExprPtr Parser::parseNot() {
    if (check(TokenType::KEYWORD, "not")) {
        const Token& opTok = advance();
        auto operand = parseNot();  // not has the highest precedence over or/and
        return make_unique<UnaryExpr>("not", move(operand), opTok.line, opTok.column);
    }
    return parseComparison();
}

ExprPtr Parser::parseComparison() {
    auto left = parseAdditive();
    if (isComparisonOp(current())) {
        const Token& opTok = advance();
        auto right = parseAdditive();
        return make_unique<BinaryExpr>(opTok.lexeme, move(left), move(right),
                                       opTok.line, opTok.column);
    }
    return left;
}

ExprPtr Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::OPERATOR, "+") || check(TokenType::OPERATOR, "-")) {
        const Token& opTok = advance();
        auto right = parseMultiplicative();
        left = make_unique<BinaryExpr>(opTok.lexeme, move(left), move(right),
                                       opTok.line, opTok.column);
    }
    return left;
}

ExprPtr Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::OPERATOR, "*") || check(TokenType::OPERATOR, "/") ||
           check(TokenType::OPERATOR, "%")) {
        const Token& opTok = advance();
        auto right = parseUnary();
        left = make_unique<BinaryExpr>(opTok.lexeme, move(left), move(right),
                                       opTok.line, opTok.column);
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::OPERATOR, "-") || check(TokenType::OPERATOR, "+")) {
        bool minus = check(TokenType::OPERATOR, "-");
        const Token& opTok = advance();
        auto operand = parseUnary();
        if (!minus) return operand;  // unary plus has no effect
        return make_unique<UnaryExpr>(opTok.lexeme, move(operand), opTok.line, opTok.column);
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    const Token& tok = current();

    if (check(TokenType::INTEGER)) {
        advance();
        return make_unique<NumberExpr>(tok.lexeme, false, tok.line, tok.column);
    }
    if (check(TokenType::FLOAT)) {
        advance();
        return make_unique<NumberExpr>(tok.lexeme, true, tok.line, tok.column);
    }
    if (check(TokenType::TIME_LITERAL)) {
        advance();
        return make_unique<TimeExpr>(tok.lexeme, tok.line, tok.column);
    }
    if (check(TokenType::KEYWORD, "True") || check(TokenType::KEYWORD, "False")) {
        bool value = tok.lexeme == "True";
        advance();
        return make_unique<BoolExpr>(value, tok.line, tok.column);
    }
    if (check(TokenType::PUNCTUATION, "(")) {
        advance();
        auto expr = parseExpression();
        expect(TokenType::PUNCTUATION, ")");
        return expr;
    }
    if (check(TokenType::IDENTIFIER)) {
        advance();
        ExprPtr expr = make_unique<VarExpr>(tok.lexeme, tok.line, tok.column);

        // Identifier suffix: function call f(...), index a[i], or member access obj.attr
        while (true) {
            if (check(TokenType::PUNCTUATION, "(")) {
                auto var = dynamic_cast<VarExpr*>(expr.get());
                if (!var) parseError(current(), "only named functions can be called");
                advance();
                vector<ExprPtr> args;
                if (!check(TokenType::PUNCTUATION, ")")) {
                    args.push_back(parseExpression());
                    while (check(TokenType::PUNCTUATION, ",")) {
                        advance();
                        args.push_back(parseExpression());
                    }
                }
                expect(TokenType::PUNCTUATION, ")");
                expr = make_unique<CallExpr>(var->name, move(args), tok.line, tok.column);
            }
            else if (check(TokenType::PUNCTUATION, "[")) {
                if (!dynamic_cast<VarExpr*>(expr.get()))
                    parseError(current(), "indexing is only supported on variables");
                advance();
                auto index = parseExpression();
                expect(TokenType::PUNCTUATION, "]");
                string arrayName = dynamic_cast<VarExpr*>(expr.get())->name;
                expr = make_unique<IndexExpr>(arrayName, move(index), tok.line, tok.column);
            }
            else if (check(TokenType::PUNCTUATION, ".")) {
                if (!dynamic_cast<VarExpr*>(expr.get()))
                    parseError(current(), "attribute access is only supported on variables");
                advance();
                const Token& attrTok = expect(TokenType::IDENTIFIER);
                string objectName = dynamic_cast<VarExpr*>(expr.get())->name;
                expr = make_unique<AttributeExpr>(objectName, attrTok.lexeme,
                                                  tok.line, tok.column);
            }
            else {
                break;
            }
        }

        return expr;
    }
    if (check(TokenType::KEYWORD, "range")) {
        parseError(tok, "'range' is only valid inside a for statement");
    }
    parseError(tok, "unexpected token '" + tok.lexeme + "'");
}

