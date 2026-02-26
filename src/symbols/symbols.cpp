#include "symbols.h"
#include "../common/color.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cerrno>

#include <elf.h>

static char get_symbol_type(uint8_t info)
{
    uint8_t type = ELF64_ST_TYPE(info); // only right 4 byte(function or object) the left is private or public

    if (type == STT_FUNC)
    {
        return 'F';
    }
    if (type == STT_OBJECT)
    {
        return 'O';
    }
    else
    {
        return '?';
    }
}

static vector<Symbol> read_symbols_64(ifstream &file, const Elf64_Ehdr &ehdr)
{
    vector<Symbol> result;

    vector<Elf64_Shdr> shdrs(ehdr.e_shnum); // e_shnum how many section in the ELF , shdr is a section in ELF
    file.seekg(ehdr.e_shoff, ios::beg);     // e_shoff , the section store address , seekg go to the page , ios::beg start at the beginging of the file
    file.read(reinterpret_cast<char *>(shdrs.data()), ehdr.e_shnum * sizeof(Elf64_Shdr));
    //.data store in shdr , reinterpret_cast change type , how amny section * section size

    Elf64_Shdr &shstrtab_hdr = shdrs[ehdr.e_shstrndx]; // get the string symbol

    vector<char> shstrtab(shstrtab_hdr.sh_size); // how many bytes
    file.seekg(shstrtab_hdr.sh_offset, ios::beg);
    file.read(shstrtab.data(), shstrtab_hdr.sh_size);

    Elf64_Shdr *symtab_hdr = nullptr;
    Elf64_Shdr *strtab_hdr = nullptr;

    for (auto &shdr : shdrs)
    {                                               // check all the shdrs
        const char *name = &shstrtab[shdr.sh_name]; // shdr.sh_name the section name , find it in shstrtab

        if (strcmp(name, "symtab") == 0)
        {
            symtab_hdr = &shdr;
        }
        else if (strcmp(name, ".strtab") == 0)
        {
            strtab_hdr = &shdr;
        }
    }

    // if there is no symtab , the program may delete it
    if (symtab_hdr == nullptr)
    {
        for (auto &shdr : shdrs)
        {
            const char *name = &shstrtab[shdr.sh_name];

            if (strcmp(name, ".dynsym") == 0)
            {
                symtab_hdr = &shdr;
            }
            else if (strcmp(name, ".dynstr") == 0)
            {
                strtab_hdr = &shdr;
            }
        }
    }

    if (symtab_hdr == nullptr || strtab_hdr == nullptr)
    {
        cerr << "[Symbols] :Error , symbol table not found" << endl;
        return result;
    }

    vector<char> strtab(strtab_hdr->sh_size); // the function name , strtab_hdr is a pointer use  -> to see the value
    file.seekg(strtab_hdr->sh_offset, ios::beg);
    file.read(strtab.data(), strtab_hdr->sh_size);

    size_t sym_count = symtab_hdr->sh_size / sizeof(Elf64_Sym); // each string is 24 bytes(Elf64_Sym)
    vector<Elf64_Sym> syms(sym_count);                          // elf64_sym the at_value(memory , address)
    file.read(reinterpret_cast<char *>(syms.data()), symtab_hdr->sh_size);

    for (const auto &sym : syms)
    {
        if (sym.st_name == 0)
        {
            continue;
        }
        if (sym.st_value == 0)
        {
            continue;
        }

        Symbol s;
        s.name = &strtab[sym.st_name];
        s.offset = sym.st_value;
        s.size = sym.st_size;
        s.type = get_symbol_type(sym.st_info);

        result.push_back(s);
    }

    return result;
}

vector<Symbol> read_symbols(const string &target_path)
{
    ifstream file(target_path, ios::binary); // use binary to open the file
    if (!file)
    {
        cerr << "[Symbols] Error " << strerror(errno) << endl;
        return {};
    }

    unsigned char ident[EI_NIDENT]; //"EI_NIDENT" means 16 bytes
    file.read(reinterpret_cast<char *>(ident), EI_NIDENT);
    //"reinterpret" change unsigned char to char* , "read" and "EI_NIDENT" get 16 bytes from file

    if (!file || file.gcount() != EI_NIDENT)
    {
        //"gcount" how many bytes has been getten
        cerr << "[symbol] Error the file too short" << endl;
        return {};
    }

    if (memcmp(ident, ELFMAG, SELFMAG) != 0)
    {
        cerr << "[symbol] Error not an ELF" << endl;
        return {};
    }

    file.seekg(0, ios::beg); // start from 0 , ios::beg means the start

    if (ident[EI_CLASS] == ELFCLASS64)
    {                      // 64 byte
        Elf64_Ehdr ehdr{}; // a struct store the beinging ELF information , {} means itʼs none value avoid trash value
        file.read(reinterpret_cast<char *>(&ehdr), sizeof(ehdr));
        return read_symbols_64(file, ehdr);
    }
    else
    {
        cerr << "[Symbol]: Error not an 64 bytes file" << endl;
        return {};
    }
}

// here also can add a function to print the ELF  offsets (not actual address)

void print_symbol_addresses(const vector<Symbol> &symbols, uint64_t base_address, bool is_pie)
{

    vector<Symbol> funcs;
    for (const auto s : symbols)
    {
        if (s.type == 'F')
        {
            funcs.push_back(s);
        }
    }

    sort(funcs.begin(), funcs.end(),
         [](const Symbol &a, const Symbol &b)
         {
             return a.offset < b.offset;
         });

    for (const auto &s : funcs)
    {
        uint64_t actual_addr;

        if (is_pie)
        {
            actual_addr = base_address + s.offset;
        }
        else
        {
            actual_addr = s.offset; // Non-PIE 不用加
        }
        cout << Color::BOLD_SKY_BLUE << " 0x" << hex << setw(16) << setfill('0') << actual_addr << Color::RESET;
        cout << " : " << s.name << endl;
    }

    cout << dec << setfill(' ') << endl;
}
