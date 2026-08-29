#pragma once

#include <string>
#include "ast/ast.h"
#include "config/config_parser.h"

// Generates SCL (Structured Control Language) code for TIA Portal V19.
// Output: a Function Block (FB) with static timer/counter instances.
std::string generateScl(const Program& program, const Config& config);