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

void print_registers();

void handle_breakpoint();
void remove_breakpoint(long addr);

void set_breakpoint(long addr);

void disassemble_current_instruction(pid_t pid);
