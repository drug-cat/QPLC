#pragma once

// Simple Language Server Protocol (LSP) server for QPLC.
// Communication: JSON-RPC over stdio. Each message is delimited by Content-Length header.
// Capabilities: initialize, textDocument/publishDiagnostics (basic), shutdown, exit.

#include <string>
#include <vector>

namespace qplc::lsp {

// Runs the LSP server in the current process (until exit is called). Never returns.
void runLspServer();

// Detects errors in a QPLC source and converts them to a list of diagnostics.
// Each diagnostic: {line (1-based), col (1-based), message}.
struct Diagnostic {
    int line;
    int col;
    std::string message;
};

std::vector<Diagnostic> collectDiagnostics(const std::string& source);

} // namespace qplc::lsp
