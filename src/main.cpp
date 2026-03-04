#include <iostream>
#include <cstdlib>
#include <vector>

#include "process/process.h"
#include "memory/memory.h"
#include "common/color.h"
#include "elf/elf_view.h"
#include "symbols/symbols.h"

using namespace std;

int main(int argc, char *argv[])
{
    using namespace std;
    if (argc != 2)
    {
        print_how_to_use(argv[0]); // argv[0] = program name
        exit(1);
    }
    const char *target_path = argv[1];

    if (!print_elf_info(target_path))
    {
        cerr << "[main]: Target is not a valid ELF";
        return -1;
    }

    vector<Symbol> symbols = read_symbols(target_path);

    if (symbols.empty())
    {
        cerr << "No symbol found" << endl;
        return -1;
    }

    pid_t child_pid = start_child_and_father_program(target_path);
    if (child_pid == -1)
    {
        cerr << "Error" << endl;
        exit(1);
    }

    vector<MemoryRegion> regions = read_memory_maps(child_pid);

    print_colored_maps(regions);

    uint64_t base_address = get_base_address(regions, target_path);

    bool is_pie = check_is_pie(target_path);

    print_symbol_addresses(symbols, base_address, is_pie);

    clean_child_program(child_pid);
    return 0;
}
