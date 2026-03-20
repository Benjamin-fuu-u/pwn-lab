#pragma once

#include <vector>
#include <cstdint>
#include <string>

#include "debugger.h"
#include "../symbols/symbols.h"

using namespace std;

void run_debugger_cli(MiniDebugger &dbg, const vector<Symbol> &symbols, uint64_t base_address);
