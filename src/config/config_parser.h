#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct IoMapping {
    std::string address;
    std::string type;       // BOOL, INT, REAL, TIME
    int arrayLength;        // 1 = scalar, >1 = array length
    IoMapping() : arrayLength(1) {}
};

struct HardwareConfig {
    std::string cpu;
    std::string ip;
};

struct Config {
    HardwareConfig hardware;
    std::unordered_map<std::string, IoMapping> io;
    // Program constants from [constants] section: name → raw value (number/float/T#time)
    std::unordered_map<std::string, std::string> constants;
};

struct ConfigError {
    int line;
    std::string message;
};

Config parseConfig(const std::string& source);
Config parseConfigWithErrors(const std::string& source, std::vector<ConfigError>& errors);