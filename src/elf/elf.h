#pragma once

#include <elf.h>

typedef struct func_symbol {
    char* identifier;
    Elf64_Addr value;

    struct func_symbol *next;

} func_symbol_t;


typedef struct elf_info {
    func_symbol_t* func_symbols;
    
    enum linkage {
        LINK_STATIC  = 0x00,
        LINK_DYNAMIC = 0x01
    } link_type;

    char* interpreter;

} elf_info_t;

typedef struct elf_file {
    char* path;
    elf_info_t *info;
} elf_file_t;

elf_file_t* elf_file_fill(char*);

elf_info_t* elf_process_file(char*);
elf_info_t* elf_process_fd(int);

void print_func_symbol(const func_symbol_t*);
void print_func_symbols(const func_symbol_t*);

func_symbol_t *resolve_func_symbol_byid(const func_symbol_t*, char*);
func_symbol_t *resolve_func_symbol_byvalue(const func_symbol_t*, Elf64_Addr);

void free_func_symbols(func_symbol_t*);
