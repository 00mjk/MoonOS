#ifndef ASM_ARCH_X86_H
#define ASM_ARCH_X86_H

/*
    x86.h - Takes all the assembly functions and provides them in a single header file for ease-of-access
*/

#include <libk/kstring.h>
#include "../../drivers/vga/vga.h"
#include "../../common.h"

//Assembly labels (must be prefixed with ASM_<ARCHITECTURE>):
extern const char *ASM_x86_cpuid_vendor_string      ();
extern uint64_t    ASM_x86_cpuid_check_bi_local_apic();
extern int         ASM_x86_regs_read_rip();

//C functions:
void x86_cpuid_vendor_string(const char *a, const char *b, const char *c);
void x86_cpu_info();

#endif // ASM_ARCH_X86_H