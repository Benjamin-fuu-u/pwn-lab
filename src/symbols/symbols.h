#pragma

#include <string>
#include <vector>
#include <cstdint>

using namespace std;

struct Symbol
{
    string name;
    uint64_t offset;
    uint64_t size;
    char type;
};

vector<Symbol> read_symbols(const string &target_path);

void print_symbol_addresses(const vector<Symbol> &symbols, uint64_t base_address, bool is_pie);

// uint64_t find_symbol_offset(const vector<Symbol> &symbols, const string &name);
