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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: qplc <conf.qplc> <source.q>\n";
        return 1;
    }

    // ---------- 1. Read config file ----------
    std::ifstream confFile(argv[1]);
    if (!confFile) {
        std::cerr << "Error: Cannot open config file " << argv[1] << "\n";
        return 1;
    }
    std::stringstream confBuffer;
    confBuffer << confFile.rdbuf();
    std::string confSource = confBuffer.str();

    std::vector<ConfigError> configErrors;
    Config config = parseConfigWithErrors(confSource, configErrors);
    if (!configErrors.empty()) {
        std::cerr << "Config errors:\n";
        for (const auto& err : configErrors) {
            std::cerr << "  Line " << err.line << ": " << err.message << "\n";
        }
        return 1;
    }

    // ---------- 2. Read program file ----------
    std::ifstream progFile(argv[2]);
    if (!progFile) {
        std::cerr << "Error: Cannot open program file " << argv[2] << "\n";
        return 1;
    }
    std::stringstream progBuffer;
    progBuffer << progFile.rdbuf();
    std::string progSource = progBuffer.str();

    // ---------- 3. Compile pipeline ----------
    try {
        // Tokenize
        auto tokens = tokenize(progSource);

        // Parse
        Parser parser(tokens);
        auto program = parser.parseProgram();

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

        // Output XML to stdout (only XML goes to stdout)
        std::cout << ladderXml;

        // Success message to stderr (doesn't pollute XML)
        std::cerr << "Parsed, analyzed, and generated successfully!\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}