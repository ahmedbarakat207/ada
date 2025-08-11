#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/user.h>
#include <errno.h>
#include <stdint.h>
#include <bits/signum-generic.h>
#include "debugger.h"
#include "commands.h"
#include "defines.h"
#include <capstone/capstone.h>


#define MAX_BREAKPOINTS 10

typedef struct {
    long addr;
    uint8_t orig_byte;
    int is_set;
} Breakpoint;

extern pid_t child_pid;
static Breakpoint breakpoints[MAX_BREAKPOINTS];
static int breakpoint_count = 0;

void print_registers() {
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, child_pid, 0, &regs) == -1) {
        perror("ptrace(GETREGS)");
        return;
    }

    printf("RAX: 0x%lx\n", regs.rax);
    printf("RBX: 0x%lx\n", regs.rbx);
    printf("RCX: 0x%lx\n", regs.rcx);
    printf("RDX: 0x%lx\n", regs.rdx);
    printf("RSI: 0x%lx\n", regs.rsi);
    printf("RDI: 0x%lx\n", regs.rdi);
    printf("RBP: 0x%lx\n", regs.rbp);
    printf("RSP: 0x%lx\n", regs.rsp);
    printf("RIP: 0x%lx\n", regs.rip);
    printf("R8: 0x%lx\n", regs.r8);
    printf("R9: 0x%lx\n", regs.r9);
    printf("R10: 0x%lx\n", regs.r10);
    printf("R11: 0x%lx\n", regs.r11);
    printf("R12: 0x%lx\n", regs.r12);
    printf("R13: 0x%lx\n", regs.r13);
    printf("R14: 0x%lx\n", regs.r14);
    printf("R15: 0x%lx\n", regs.r15);
}

int find_breakpoint(long addr) {
    for (int i = 0; i < breakpoint_count; i++) {
        if (breakpoints[i].addr == addr) {
            return i;
        }
    }
    return -1;
}

void remove_breakpoint_at_index(int index) {
    Breakpoint *bp = &breakpoints[index];
    if (!bp->is_set) return;

    long aligned_addr = bp->addr & ~0x7;
    int offset = bp->addr - aligned_addr;
    long word = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)aligned_addr, 0);
    if (word == -1) {
        perror("ptrace(PEEKTEXT)");
        return;
    }

    uint8_t *bytes = (uint8_t*)&word;
    bytes[offset] = bp->orig_byte;
    if (ptrace(PTRACE_POKETEXT, child_pid, (void*)aligned_addr, word) == -1) {
        perror("ptrace(POKETEXT)");
        return;
    }

    bp->is_set = 0;
    printf("Breakpoint removed at 0x%lx\n", bp->addr);
}

void remove_breakpoint(long addr){
    int index = find_breakpoint(addr);
    if (index == -1) {
        printf("No breakpoint at 0x%lx\n", addr);
        return;
    }
    remove_breakpoint_at_index(index);
}

void delete_breakpoint(long addr) {
    int index = find_breakpoint(addr);
    if (index == -1) {
        printf("No breakpoint at 0x%lx\n", addr);
        return;
    }
    
    if (breakpoints[index].is_set) {
        remove_breakpoint_at_index(index);
    }
    
    // Shift array to remove breakpoint
    for (int i = index; i < breakpoint_count - 1; i++) {
        breakpoints[i] = breakpoints[i+1];
    }
    breakpoint_count--;
    printf("Breakpoint deleted at 0x%lx\n", addr);
}

void set_breakpoint(long addr) {
    // Check if already exists
    if (find_breakpoint(addr) != -1) {
        printf("Breakpoint already exists at 0x%lx\n", addr);
        return;
    }
    
    if (breakpoint_count >= MAX_BREAKPOINTS) {
        printf("Maximum breakpoints reached\n");
        return;
    }

    long aligned_addr = addr & ~0x7;
    int offset = addr - aligned_addr;
    long word = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)aligned_addr, 0);
    if (word == -1) {
        perror("ptrace(PEEKTEXT)");
        return;
    }

    uint8_t *bytes = (uint8_t*)&word;
    uint8_t orig_byte = bytes[offset];
    bytes[offset] = 0xCC;  // INT3 instruction

    if (ptrace(PTRACE_POKETEXT, child_pid, (void*)aligned_addr, word) == -1) {
        perror("ptrace(POKETEXT)");
        return;
    }

    // Add new breakpoint
    breakpoints[breakpoint_count].addr = addr;
    breakpoints[breakpoint_count].orig_byte = orig_byte;
    breakpoints[breakpoint_count].is_set = 1;
    breakpoint_count++;
    printf("Breakpoint set at 0x%lx\n", addr);
}

void reinsert_all_breakpoints() {
    for (int i = 0; i < breakpoint_count; i++) {
        Breakpoint *bp = &breakpoints[i];
        if (bp->is_set) continue;

        long aligned_addr = bp->addr & ~0x7;
        int offset = bp->addr - aligned_addr;
        long word = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)aligned_addr, 0);
        if (word == -1) {
            perror("ptrace(PEEKTEXT)");
            continue;
        }

        uint8_t *bytes = (uint8_t*)&word;
        bytes[offset] = 0xCC;  // Reinsert INT3
        
        if (ptrace(PTRACE_POKETEXT, child_pid, (void*)aligned_addr, word) == -1) {
            perror("ptrace(POKETEXT)");
            continue;
        }
        
        bp->is_set = 1;
        printf("Breakpoint reinserted at 0x%lx\n", bp->addr);
    }
}

void handle_breakpoint(long addr) {
    int index = find_breakpoint(addr);
    if (index == -1) {
        printf("Unknown breakpoint at 0x%lx\n", addr);
        return;
    }

    Breakpoint *bp = &breakpoints[index];
    if (!bp->is_set) {
        printf("Breakpoint not active at 0x%lx\n", addr);
        return;
    }

    // Remove breakpoint temporarily
    remove_breakpoint_at_index(index);

    // Reset RIP to original instruction
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, child_pid, 0, &regs) == -1) {
        perror("ptrace(GETREGS)");
        return;
    }

    regs.rip = addr;
    if (ptrace(PTRACE_SETREGS, child_pid, 0, &regs) == -1) {
        perror("ptrace(SETREGS)");
        return;
    }

    printf("Breakpoint hit at 0x%lx\n", addr);
}



void disassemble_current_instruction(pid_t pid) {
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
        perror("ptrace(GETREGS) failed");
        return;
    }
    if (regs.rip == 0) {
        printf("RIP is zero, cannot disassemble\n");
        return;
    }
    unsigned long addr = regs.rip;
    const int max_insn_size = 15;
    unsigned char code[max_insn_size];
    
    memset(code, 0, max_insn_size);

    for (int i = 0; i < max_insn_size; i += sizeof(long)) {
        long word = ptrace(PTRACE_PEEKTEXT, pid, addr + i, 0);
        if (word == -1) {
            perror("ptrace(PEEKTEXT) failed");
            break;
        }
        

        size_t bytes_left = max_insn_size - i;
        size_t copy_size = (bytes_left < sizeof(long)) ? bytes_left : sizeof(long);
        memcpy(code + i, &word, copy_size);
    }

    printf("  0x%lx: ", addr);
    for (int i = 0; i < max_insn_size; i++) {
        printf("%02x ", code[i]);
    }
    printf("\n");
}
