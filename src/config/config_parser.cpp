#include "config/config_parser.h"

#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstdlib>

using namespace std;

namespace {

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

}  // namespace

Config parseConfig(const string& source) {
    vector<ConfigError> errors;
    return parseConfigWithErrors(source, errors);
}

Config parseConfigWithErrors(const string& source, vector<ConfigError>& errors) {
    Config config;
    istringstream input(source);
    string line;
    int lineNum = 0;
    string currentSection;

    while (getline(input, line)) {
        lineNum++;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        if (trimmed[0] == '[' && trimmed.back() == ']') {
            currentSection = trimmed.substr(1, trimmed.size() - 2);
            continue;
        }

        size_t eq = trimmed.find('=');
        if (eq == string::npos) {
            errors.push_back({lineNum, "Expected '=' in line: " + trimmed});
            continue;
        }

        string key = trim(trimmed.substr(0, eq));
        string value = trim(trimmed.substr(eq + 1));

        if (currentSection == "hardware") {
            if (key == "cpu") config.hardware.cpu = value;
            else if (key == "ip") config.hardware.ip = value;
        } else if (currentSection == "io") {
            // value must be address:type[arrayLength]
            size_t colon = value.rfind(':');
            if (colon == string::npos) {
                errors.push_back({lineNum, "IO mapping '" + key + "' must be address:type"});
                continue;
            }
            IoMapping mapping;
            mapping.address = trim(value.substr(0, colon));
            string typePart = trim(value.substr(colon + 1));

            // بررسی آرایه
            size_t bracket = typePart.find('[');
            if (bracket != string::npos) {
                size_t closeBracket = typePart.find(']', bracket);
                if (closeBracket == string::npos) {
                    errors.push_back({lineNum, "Missing ']' in type for '" + key + "'"});
                    continue;
                }
                mapping.type = trim(typePart.substr(0, bracket));
                string lenStr = trim(typePart.substr(bracket + 1, closeBracket - bracket - 1));
                if (lenStr.empty()) {
                    errors.push_back({lineNum, "Array length missing for '" + key + "'"});
                    continue;
                }
                mapping.arrayLength = stoi(lenStr);
                if (mapping.arrayLength <= 1) {
                    errors.push_back({lineNum, "Array length must be > 1 for '" + key + "'"});
                    continue;
                }
            } else {
                mapping.type = typePart;
                mapping.arrayLength = 1;
            }

            if (mapping.address.empty() || mapping.type.empty()) {
                errors.push_back({lineNum, "Invalid IO mapping for '" + key + "'"});
                continue;
            }

            if (config.io.find(key) != config.io.end()) {
                errors.push_back({lineNum, "Duplicate IO variable '" + key + "'"});
                continue;
            }

            config.io[key] = mapping;
        }
    }

    return config;
}