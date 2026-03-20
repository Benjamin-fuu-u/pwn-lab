#include <iostream>
#include <iomanip>
#include <cstring>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "debugger.h"
#include "../common/color.h"

#include <capstone/capstone.h>

using namespace std;

MiniDebugger::MiniDebugger(pid_t pid)
    : m_pid(pid) // initialization m_pid to pid
      ,
      m_bp_addr(0) // define the variable
      ,
      m_bp_backup(0), m_bp_set(false)
{
}

void MiniDebugger::read_memory(uint64_t addr, uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i += sizeof(long))
    {
        long word = ptrace(PTRACE_PEEKTEXT, m_pid, addr + i, nullptr);
        // get child program address , long means 8 bytes
        size_t to_copy = min(sizeof(long), len - i);
        // choose how many data sshould move(the ptrace read 8 byte and the last space) , choose the small to move
        memcpy(buf + i, &word, to_copy); // move word to buf , move to_copy size
    }
}

uint64_t MiniDebugger::get_rip()
{
    struct user_regs_struct regs; // the struct defined by linux (all the rip)
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);

    return regs.rip;
}

void MiniDebugger::set_breakpoint(uint64_t addr)
{
    m_bp_addr = addr;

    m_bp_backup = ptrace(PTRACE_PEEKTEXT, m_pid, addr, nullptr);
    // back up origin machine code 8 byte like 0x1122334455667788
    // and the start is 88 second is 77

    long modified = (m_bp_backup & ~0xFF) | 0xcc;
    // change the last byte (0x1122334455667788) to 0x1122334455667700  and fill cc
    // 0x11223344556677cc

    ptrace(PTRACE_POKETEXT, m_pid, addr, modified); // write the chanf address back

    m_bp_set = true; // breakpoint set

    cout << Color::BOLD_LIME_GREEN << "[Debugger] Breakpoint set at 0x" << hex << addr << dec << endl
         << Color::RESET;
}

bool MiniDebugger::run_to_breakpoint()
{
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr); //"CONT" run until breakpoint

    int status;
    waitpid(m_pid, &status, 0); // wait when the child program send single (0xcc or error)

    if (WIFEXITED(status))
    { // if true the child program end
        cout << "[Debugger] Child program exited" << endl;
        return false;
    }

    //***  important
    ptrace(PTRACE_POKETEXT, m_pid, m_bp_addr, m_bp_backup); // write origin command back

    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs); // get the breakpoint regs
    regs.rip = m_bp_addr;                          // the kernel has point the 0xcc next Assembly changed it to the breakpoint addrss
    ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs); // write the regs back

    cout << Color::BOLD_CORAL_RED << "[Debugger] Hit breakpoint at ox" << hex << m_bp_addr << dec << Color::RESET << endl;
    return true;
}

bool MiniDebugger::single_step()
{
    ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr); // run one Assembly

    int status;
    waitpid(m_pid, &status, 0); // until it end

    if (WIFEXITED(status))
    {
        cout << "[Debugger] Program existed" << endl;
        return false;
    }

    return true;
}
void MiniDebugger::print_registers()
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);

    cout << "-----Registers-----" << endl;

    cout << "RIP = " << Color::BOLD_LAVENDER << "0x" << hex << regs.rip << Color::RESET << " where is the program" << endl;
    cout << "RSP = " << Color::BOLD_CORAL_RED << "0x" << regs.rsp << Color::RESET << " stack top" << endl;
    cout << "RBP = " << Color::BOLD_LIME_GREEN << "0x" << regs.rbp << Color::RESET << " stack bottom" << endl;

    cout << "RAX = " << Color::BOLD_ORANGE << "0x" << regs.rax << Color::RESET << " retrun value" << endl;
    cout << "RDI = " << Color::BOLD_SKY_BLUE << "0x" << regs.rdi << Color::RESET << " first arguments" << endl;
    cout << "RSI = " << Color::BOLD_SKY_BLUE << "0x" << regs.rsi << Color::RESET << " second arguments" << endl;
    cout << "RDX = " << Color::BOLD_SKY_BLUE << "0x" << regs.rdx << Color::RESET << " third arguments" << endl;
    cout << Color::RESET << dec << endl;
}

void MiniDebugger::disassemble_at_rip()
{
    uint64_t rip = get_rip();

    uint8_t code[16];
    read_memory(rip, code, sizeof(code)); // read the next assembly (16 bytes) to code

    csh handle;    // innitilize capstone
    cs_insn *insn; // store the disassembly result int insn

    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) // use ARCHX86 64 bit if error exit
    {
        cerr << "[Debugger] Capstone init failed" << endl;
        return;
    }

    size_t count = cs_disasm(handle, code, sizeof(code), rip, 1, &insn);
    // handle capstone , code the disassemble code ,rip use whem jump or call , 1 one assemble , insn store the result

    if (count > 0)
    {                                                                         // disassembly OK
        cout << Color::BOLD_CYAN << " 0x" << hex << insn[0].address << " : "; // now instruction address

        cout << Color::BOLD_GREY;
        for (int j = 0; j < insn[0].size; j++) // how many machine instruction
        {
            cout << setw(2) << setfill('0') << (int)insn[0].bytes[j] << " "; //(int) change char to int
        }

        for (int j = insn[0].size; j < 8; j++) // if itʼs not 8 byte fill it
        {
            cout << "   ";
        }

        cout << Color::BOLD_LAVENDER;
        cout << insn[0].mnemonic << " " << insn[0].op_str; // insn.mnemonic  like mov call , insn.op_str is the operater object
        cout << Color::RESET << dec << endl;

        cs_free(insn, count); // memory release
    }
    else
    {
        cout << " 0x" << hex << rip << "???" << dec << endl;
    }
    cs_close(&handle);
}
