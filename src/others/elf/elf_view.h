#pragma once

#include <string>

using namespace std;

bool print_elf_info(const string &target_path);

bool check_is_pie(const string &target_path);
