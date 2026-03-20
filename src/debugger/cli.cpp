#include <iostream>
#include <sstream>

#include "cli.h"
#include "../common/color.h"

using namespace std;

static bool parse_hex_u64(const string &s, uint64_t &out)
{
    try // if correct
    {
        size_t idx = 0;
        out = stoull(s, &idx, 16); // change s to hex store the result at idx
        return idx == s.size();
    }
    catch (...) // if any error
    {
        return false;
    }
}

static void print_help()
{
    cout << " ===== [Debugger Commands] ===== " << endl
         << endl;

    cout << "s [N]     : single step N and show the registers" << endl;
    cout << "b <addr>  : set breakpoint at address (hex)" << endl;
    cout << "bm <name> : set breakpoint at symbol name" << endl;
    cout << "c         : continue until reach breakpoint" << endl;
    cout << "regs      : print registers" << endl;
    cout << "dis       : disassemble at now RIP" << endl;
    cout << "help      : show help" << endl;
    cout << "q         : quit" << endl;
}

void run_debugger_cli(MiniDebugger &dbg, const vector<Symbol> &symbols, uint64_t base_address)
{
    print_help();

    string line;
    while (true)
    {
        cout << Color::BOLD_ORANGE << "dbg >>" << Color::RESET << endl;

        if (!getline(cin, line)) // if input is EOF
            break;
        if (line.empty())
            continue;

        istringstream iss(line);
        string cmd;
        iss >> cmd; // get the first input

        if (cmd == "q")
            break;
        else if (cmd == "help")
            print_help();
        else if (cmd == "regs")
            dbg.print_registers();
        else if (cmd == "dis")
            dbg.disassemble_at_rip();
        else if (cmd == "s")
        {
            int n = 1;
            if (!(iss >> n)) // if the input stream is empty n = 1
                n = 1;
            if (n < 0)
                n = 1;

            for (int i = 0; i < n; i++)
            {
                dbg.print_registers();
                dbg.disassemble_at_rip();

                if (!dbg.single_step())
                {
                    cout << "[Debugger] target exit" << endl;
                    return;
                }
            }
        }

        else if (cmd == "b")
        {
            string addr_str;
            if (!(iss >> addr_str))
            {
                cout << "[Debugger] usage: d <hex_addr>" << endl;
                continue;
            }

            uint64_t addr = 0;
            if (!parse_hex_u64(addr_str, addr))
            {
                cout << "[Debugger] invalid hex address" << endl;
                continue;
            }

            dbg.set_breakpoint(addr);
            cout << "[Debugger] breakpoint set at 0x" << hex << addr << dec << endl;
        }
        else if (cmd == "bm")
        {
            string name;
            if (!(iss >> name)) // no function name
            {
                cout << "[Debugger] usage: bm <symbol_name>" << endl;
                continue;
            }

            uint64_t off = find_symbol_offset(symbols, name);
            if (off == 0)
            {
                cout << "[Debugger] symbol not found : " << name << endl;
                continue;
            }

            uint64_t addr = base_address + off;
            dbg.set_breakpoint(addr);
            cout << "[Debugger] breakpoint set at " << name << " 0x" << hex << addr << dec << endl;
        }
        else if (cmd == "c")
        {
            if (!dbg.run_to_breakpoint())
            {
                cout << "[Debugger] target exited" << endl;
                return;
            }
            dbg.print_registers();
            dbg.disassemble_at_rip();
        }
        else
        {
            cout << "[Debugger] unknown command: " << cmd << endl;
            cout << "Type 'help' for usage" << endl;
        }
    }
}