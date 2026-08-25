#pragma once

#include <string>
#include "ast/ast.h"
#include "config/config_parser.h"

std::string generateLadderXml(const Program& program, const Config& config);