#define _GNU_SOURCE
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
#include <stdbool.h>

bool breakpoint_active = false;
long breakpoint_addr = 0;
unsigned char breakpoint_orig_byte = 0;
bool breakpoint_is_set = false;


#define MAX_BREAKPOINTS 10

int status;

int main(int argc, char **argv){
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return 1;
    }
    printf("   █████████   ██████████     █████████  \n");
    printf("  ███░░░░░███ ░░███░░░░███   ███░░░░░███ \n");
    printf(" ░███    ░███  ░███   ░░███ ░███    ░███ \n");
    printf(" ░███████████  ░███    ░███ ░███████████ \n");
    printf(" ░███░░░░░███  ░███    ░███ ░███░░░░░███ \n");
    printf(" ░███    ░███  ░███    ███  ░███    ░███ \n");
    printf(" █████   █████ ██████████   █████   █████\n");
    printf("░░░░░   ░░░░░ ░░░░░░░░░░   ░░░░░   ░░░░░ \n");
    printf("       Welcome to the Ada Debugger       \n");
    printf("         Made By : Ahmed Barakat         \n");
    printf("         Github: ahmedbarakat207         \n");
    
    run_cli(argv[1], &argv[1]);

    return 0;
}