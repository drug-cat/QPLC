#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "config/config_parser.h"
#include "semantic/semantic_analyzer.h"
#include "codegen/ladder_generator.h"
#include "codegen/scl_generator.h"
#include "lsp/lsp_server.h"

// ------------------- Debug dumps (--tokens / --ast) -------------------

static void printTokens(const std::vector<Token>& tokens) {
    for (const auto& tok : tokens) {
        std::cout << tokenTypeToString(tok.type);
        if (!tok.lexeme.empty()) {
            std::cout << " '" << tok.lexeme << "'";
        }
        std::cout << "  @" << tok.line << ":" << tok.column << "\n";
    }
}

static void printIndent(int depth) {
    for (int i = 0; i < depth; ++i) std::cout << "  ";
}

static void printExpr(const Expr& expr, int depth);

static void printExpr(const Expr& expr, int depth) {
    printIndent(depth);
    if (auto num = dynamic_cast<const NumberExpr*>(&expr)) {
        std::cout << "Number(" << num->value << (num->isFloat ? "f" : "") << ")\n";
    } else if (auto b = dynamic_cast<const BoolExpr*>(&expr)) {
        std::cout << "Bool(" << (b->value ? "True" : "False") << ")\n";
    } else if (auto t = dynamic_cast<const TimeExpr*>(&expr)) {
        std::cout << "Time(" << t->value << ")\n";
    } else if (auto v = dynamic_cast<const VarExpr*>(&expr)) {
        std::cout << "Var(" << v->name << ")\n";
    } else if (auto idx = dynamic_cast<const IndexExpr*>(&expr)) {
        std::cout << "Index(" << idx->name << ") @line " << idx->line << "\n";
        if (idx->index) printExpr(*idx->index, depth + 1);
    } else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        std::cout << "Call(" << call->funcName << ")\n";
        for (const auto& arg : call->args) {
            if (arg) printExpr(*arg, depth + 1);
        }
    } else if (auto attr = dynamic_cast<const AttributeExpr*>(&expr)) {
        std::cout << "Attr(" << attr->objectName << "." << attr->attrName << ")\n";
    } else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        std::cout << "Unary(" << un->op << ")\n";
        if (un->operand) printExpr(*un->operand, depth + 1);
    } else if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        std::cout << "Binary(" << bin->op << ")\n";
        if (bin->left) printExpr(*bin->left, depth + 1);
        if (bin->right) printExpr(*bin->right, depth + 1);
    } else {
        std::cout << "?\n";
    }
}

static void printStmt(const Stmt& stmt, int depth);

static void printStmt(const Stmt& stmt, int depth) {
    printIndent(depth);
    if (auto assign = dynamic_cast<const AssignmentStmt*>(&stmt)) {
        std::cout << "Assign(" << assign->name << ")\n";
        if (assign->expr) printExpr(*assign->expr, depth + 1);
    } else if (auto idxAssign = dynamic_cast<const IndexAssignmentStmt*>(&stmt)) {
        std::cout << "IndexAssign(" << idxAssign->name << ")\n";
        if (idxAssign->index) printExpr(*idxAssign->index, depth + 1);
        if (idxAssign->expr) printExpr(*idxAssign->expr, depth + 1);
    } else if (auto callStmt = dynamic_cast<const CallStmt*>(&stmt)) {
        std::cout << "CallStmt(" << callStmt->funcName << ")\n";
        for (const auto& arg : callStmt->args) {
            if (arg) printExpr(*arg, depth + 1);
        }
    } else if (dynamic_cast<const BreakStmt*>(&stmt)) {
        std::cout << "Break\n";
    } else if (dynamic_cast<const ContinueStmt*>(&stmt)) {
        std::cout << "Continue\n";
    } else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        std::cout << "If\n";
        if (ifStmt->cond) printExpr(*ifStmt->cond, depth + 1);
        printIndent(depth); std::cout << "then:\n";
        for (const auto& s : ifStmt->thenBlock) printStmt(*s, depth + 1);
        for (const auto& branch : ifStmt->elifBranches) {
            printIndent(depth); std::cout << "elif:\n";
            if (branch.first) printExpr(*branch.first, depth + 1);
            for (const auto& s : branch.second) printStmt(*s, depth + 1);
        }
        if (!ifStmt->elseBlock.empty()) {
            printIndent(depth); std::cout << "else:\n";
            for (const auto& s : ifStmt->elseBlock) printStmt(*s, depth + 1);
        }
    } else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        std::cout << "While\n";
        if (whileStmt->cond) printExpr(*whileStmt->cond, depth + 1);
        for (const auto& s : whileStmt->body) printStmt(*s, depth + 1);
    } else if (auto forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        std::cout << "For(" << forStmt->varName << ", " << forStmt->start
                  << ".." << forStmt->end << ")\n";
        for (const auto& s : forStmt->body) printStmt(*s, depth + 1);
    } else {
        std::cout << "?\n";
    }
}

static void printProgram(const Program& program) {
    for (const auto& func : program.functions) {
        std::cout << "FunctionDef(" << func->name << ")";
        for (const auto& p : func->params) std::cout << " " << p;
        std::cout << "\n";
        for (const auto& s : func->body) printStmt(*s, 1);
    }
}

int main(int argc, char* argv[]) {
    // Usage: qplc <conf.qplc> <source.q> [-o output.xml] [-s output.scl] [--tokens] [--ast]
    std::string confFileName;
    std::string progFileName;
    std::string outputPath;
    std::string sclPath;
    bool dumpTokens = false;
    bool dumpAst = false;

    int positionalArgs = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lsp") {
            qplc::lsp::runLspServer();
            return 0;
        } else if (arg == "-o") {
            if (i + 1 >= argc) { std::cerr << "Error: -o requires a file path\n"; return 1; }
            outputPath = argv[++i];
        } else if (arg == "-s") {
            if (i + 1 >= argc) { std::cerr << "Error: -s requires a file path\n"; return 1; }
            sclPath = argv[++i];
        } else if (arg == "--tokens") {
            dumpTokens = true;
        } else if (arg == "--ast") {
            dumpAst = true;
        } else if (positionalArgs == 0) {
            confFileName = arg;
            positionalArgs++;
        } else if (positionalArgs == 1) {
            progFileName = arg;
            positionalArgs++;
        } else {
            std::cerr << "Usage: qplc <conf.qplc> <source.q> [-o output.xml] [-s output.scl] [--tokens] [--ast]\n";
            return 1;
        }
    }
    if (!dumpTokens && !dumpAst && positionalArgs < 2) {
        std::cerr << "Usage: qplc <conf.qplc> <source.q> [-o output.xml] [-s output.scl] [--tokens] [--ast]\n";
        return 1;
    }

    // ---------- 1. Read config file ----------
    Config config;
    if (!confFileName.empty()) {
        std::ifstream confFile(confFileName);
        if (!confFile) {
            std::cerr << "Error: Cannot open config file " << confFileName << "\n";
            return 1;
        }
        std::stringstream confBuffer;
        confBuffer << confFile.rdbuf();

        std::vector<ConfigError> configErrors;
        config = parseConfigWithErrors(confBuffer.str(), configErrors);
        if (!configErrors.empty()) {
            std::cerr << "Config errors:\n";
            for (const auto& err : configErrors) {
                std::cerr << "  Line " << err.line << ": " << err.message << "\n";
            }
            return 1;
        }
    }

    // ---------- 2. Read program file ----------
    std::ifstream progFile(progFileName);
    if (!progFile) {
        std::cerr << "Error: Cannot open program file " << progFileName << "\n";
        return 1;
    }
    std::stringstream progBuffer;
    progBuffer << progFile.rdbuf();
    std::string progSource = progBuffer.str();

    try {
        // Tokenize
        auto tokens = tokenize(progSource);

        if (dumpTokens) {
            printTokens(tokens);
            if (!dumpAst) return 0;
        }

        // Parse
        Parser parser(tokens);
        auto program = parser.parseProgram();

        if (dumpAst) {
            printProgram(*program);
            return 0;
        }

        // Semantic Analysis
        SemanticAnalyzer analyzer(config);
        auto semanticErrors = analyzer.analyze(*program);

        if (!semanticErrors.empty()) {
            std::cerr << "Semantic errors:\n";
            for (const auto& err : semanticErrors) {
                std::cerr << "  Line " << err.line << ", Col " << err.column << ": " << err.message << "\n";
            }
            return 1;
        }

        // Generate Ladder XML
        std::string ladderXml = generateLadderXml(*program, config);

        if (!outputPath.empty()) {
            // Write directly to file (UTF-8 byte-by-byte, shell encoding does not interfere)
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile) {
                std::cerr << "Error: Cannot open output file " << outputPath << "\n";
                return 1;
            }
            outFile << ladderXml;
            if (!outFile) {
                std::cerr << "Error: Failed writing output file " << outputPath << "\n";
                return 1;
            }
        } else {
            // Output XML to stdout (only XML goes to stdout)
            std::cout << ladderXml;
        }

        // Generate SCL if requested
        if (!sclPath.empty()) {
            std::string sclCode = generateScl(*program, config);
            std::ofstream sclFile(sclPath, std::ios::binary);
            if (!sclFile) {
                std::cerr << "Error: Cannot open SCL output file " << sclPath << "\n";
                return 1;
            }
            sclFile << sclCode;
            if (!sclFile) {
                std::cerr << "Error: Failed writing SCL output file " << sclPath << "\n";
                return 1;
            }
            std::cerr << "SCL generated successfully to " << sclPath << "!\n";
        }

        // Success message to stderr (doesn't pollute XML)
        std::cerr << "Parsed, analyzed, and generated successfully!\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
