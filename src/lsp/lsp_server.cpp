#include "lsp/lsp_server.h"

#include <iostream>
#include <sstream>
#include <string>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic_analyzer.h"

namespace qplc::lsp {

namespace {

// Reads a complete JSON-RPC message from stdin (with Content-Length header).
// Returns false when stdin reaches EOF.
bool readMessage(std::string& body) {
    body.clear();
    int contentLength = -1;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        if (line.rfind("Content-Length: ", 0) == 0) {
            contentLength = std::stoi(line.substr(15));
        }
    }
    if (contentLength <= 0) return false;
    body.resize(contentLength);
    std::cin.read(body.data(), contentLength);
    return !std::cin.eof() || body.size() == (size_t)contentLength;
}

// Naive JSON field extractor (only handles simple values and strings). Sufficient for LSP.
std::string extractString(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + key.size() + 3);
    if (pos == std::string::npos) return "";
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

int extractInt(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return -1;
    pos = json.find(':', pos + key.size() + 2);
    if (pos == std::string::npos) return -1;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    int n = 0;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        n = n * 10 + (json[pos] - '0');
        pos++;
    }
    return n;
}

// Simple JSON string escaping
std::string esc(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

void writeMessage(const std::string& body) {
    std::cout << "Content-Length: " << body.size() << "\r\n\r\n";
    std::cout << body;
    std::cout.flush();
}

// Sends a response to a request (with id)
void sendResponse(int id, const std::string& result) {
    std::ostringstream r;
    r << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":" << result << "}";
    writeMessage(r.str());
}

// Sends diagnostic notifications for a given URI
void publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diags) {
    std::ostringstream arr;
    arr << "[";
    for (size_t i = 0; i < diags.size(); ++i) {
        if (i > 0) arr << ",";
        arr << "{\"range\":{\"start\":{\"line\":" << (diags[i].line - 1)
            << ",\"character\":" << (diags[i].col - 1)
            << "},\"end\":{\"line\":" << (diags[i].line - 1)
            << ",\"character\":" << (diags[i].col - 1)
            << "}},\"severity\":1,\"source\":\"qplc\",\"message\":\"" << esc(diags[i].message) << "\"}";
    }
    arr << "]";
    std::ostringstream msg;
    msg << "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\","
        << "\"params\":{\"uri\":\"" << esc(uri) << "\",\"diagnostics\":" << arr.str() << "}}";
    writeMessage(msg.str());
}

} // namespace

std::vector<Diagnostic> collectDiagnostics(const std::string& source) {
    std::vector<Diagnostic> out;
    try {
        auto tokens = tokenize(source);
        Parser parser(tokens);
        auto progPtr = parser.parseProgram();
        Config emptyConfig;
        SemanticAnalyzer sem(emptyConfig);
        sem.analyze(*progPtr);
        for (const auto& e : sem.getErrors()) {
            out.push_back({e.line, e.column, e.message});
        }
    } catch (const std::exception& ex) {
        out.push_back({1, 1, ex.what()});
    }
    return out;
}

void runLspServer() {
    std::cerr << "[qplc-lsp] starting; reading JSON-RPC from stdin" << std::endl;
    std::string body;
    while (true) {
        if (!readMessage(body)) break;

        std::string method = extractString(body, "method");
        int id = extractInt(body, "id");

        if (method == "initialize") {
            sendResponse(id, "{\"capabilities\":{\"textDocumentSync\":{\"openClose\":true,\"change\":1},"
                              "\"hoverProvider\":false,\"completionProvider\":{}}}");
        } else if (method == "initialized" || method == "shutdown") {
            if (id >= 0) sendResponse(id, "null");
        } else if (method == "exit") {
            return;
        } else if (method == "textDocument/didOpen" || method == "textDocument/didChange") {
            // Extract text + uri and publish diagnostics
            std::string uri = extractString(body, "uri");
            std::string text = extractString(body, "text");
            auto diags = collectDiagnostics(text);
            publishDiagnostics(uri, diags);
        } else if (id >= 0) {
            // Empty response for unknown requests
            sendResponse(id, "null");
        }
    }
    std::cerr << "[qplc-lsp] stdin closed; exiting" << std::endl;
}

} // namespace qplc::lsp
