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
#include "commands.h"
#include "debugger.h"
#include "defines.h"



pid_t child_pid = -1;


void run_cli(const char *program, char **args) {
    child_pid = fork();
    if (child_pid == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execvp(program, args);
        perror("execvp");
        exit(1);
    }
    if (child_pid < 0) {
        perror("fork");
        return;
    }
    int status;
    waitpid(child_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Child exited\n");
        return;
    }
     while (1) {
        /*ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
        waitpid(child_pid, &status, 0); 
        if (WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            if (sig == SIGTRAP) {
                struct user_regs_struct regs;
                ptrace(PTRACE_GETREGS, child_pid, 0, &regs);

                if (breakpoint_active && breakpoint_is_set && regs.rip == breakpoint_addr + 1) {
                    handle_breakpoint();
                } else {
                    printf("SIGTRAP at 0x%lx\n", regs.rip);
                }
            } else {
                printf("Child stopped by signal %d\n", sig);
            }
        }
        if (WIFEXITED(status)) {
            printf("Child exited\n");
            break;
        }*/
        char command[128];
        printf("Debugger> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
            break;

        if (strcmp(command, "cont\n") == 0 || strcmp(command, "c\n") == 0) {
            if (breakpoint_active && !breakpoint_is_set) {
                set_breakpoint(breakpoint_addr); 
            }
            ptrace(PTRACE_CONT, child_pid, 0, 0);
        } else if (strncmp(command, "step\n", 4) == 0 || strncmp(command, "s\n", 2) == 0) {

            // this is a shitty way of typing this code but i am not touching this shit
            long steps = 1;
            int status;
            int valid_steps = 0;
    
            if (command[4] == ' ' && strlen(command) > 5) {
                if (sscanf(command + 5, "%ld", &steps) == 1) {
                            valid_steps = 1;
                        }
                }
    
            if (valid_steps) {
                if (steps <= 0) {
                    printf("Step count must be positive\n");
                } else {
                    if (steps > 1000) {
                        printf("Capping at 1000 steps\n");
                        steps = 1000;
                    }
            
                    for (long i = 0; i < steps; i++) {
                        if (ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0) == -1) {
                            perror("ptrace step failed");
                            break;
                        }
                        if (waitpid(child_pid, &status, 0) == -1) {
                            perror("waitpid failed");
                            break;
                        }
                        if (WIFEXITED(status)) {
                            printf("Child exited with status %d\n", WEXITSTATUS(status));
                            //child_exited = 1;
                            break;
                        }
                        printf("\n--- Step %ld/%ld ---\n", i+1, steps);
                        print_registers(child_pid);
                        disassemble_current_instruction(child_pid); 
                    }
                }
            } else {
                if (ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0) == -1) {
                    perror("ptrace step failed");
                } else {
                    waitpid(child_pid, &status, 0);
                    if (WIFEXITED(status)) {
                        printf("Child exited with status %d\n", WEXITSTATUS(status));
                    // child_exited = 1;
                    } else {
                        printf("\n--- Step 1/1 ---\n");
                        print_registers(child_pid);
                        disassemble_current_instruction(child_pid);
                    }
                }
            }
            continue;
        } else if (strncmp(command, "break ", 6) == 0 || strncmp(command, "b ", 2) == 0) {
            long addr;
            if (sscanf(command + 6, "%lx", &addr) == 1) {
                set_breakpoint(addr);
            } else {
                printf("Invalid address\n");
            }
            continue;
        } else if (strcmp(command, "regs\n") == 0 || strcmp(command, "r\n") == 0) {
            print_registers();
            continue;
        } else if (strcmp(command, "dbg\n") == 0 || strcmp(command, "disassemble\n") == 0 || strcmp(command, "d\n") == 0) {
            for (long i = 0; i < 100; i++) {
                if (ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0) == -1) {
                    perror("ptrace step failed");
                    break;
                }
                if (waitpid(child_pid, &status, 0) == -1) {
                    perror("waitpid failed");
                    break;
                }
                if (WIFEXITED(status)) {
                    printf("Child exited with status %d\n", WEXITSTATUS(status));
                    //child_exited = 1;
                    break;
                }
                disassemble_current_instruction(child_pid); 
            }
            continue;
        } else if (strcmp(command, "quit\n") == 0 || strcmp(command, "exit\n") == 0) {
            kill(child_pid, SIGKILL);
            break;
        } else if (strcmp(command, "remove\n") == 0 || strcmp(command, "rm\n") == 0) {
             if (breakpoint_active && breakpoint_is_set) {
                long addr;
                if (sscanf(command + 6, "%lx", &addr) == 1) {
                    remove_breakpoint(addr);
                    breakpoint_active = 0;
                    breakpoint_is_set = 0;
                } else {
                    printf("Invalid address\n");
                }
            } else {                
                printf("No breakpoint set\n");
            }
            continue;
        } else if (strcmp(command, "help\n") == 0 || strcmp(command, "h\n") == 0) {
            printf("Commands:\n");
            printf("  cont | c - continue execution\n");
            printf("  step <NUMBER OF STEPS> | s <NUMBER OF STEPS> - step to next instruction\n");
            printf("  dbg | disassmble | d - decompile steps into binary\n");
            printf("  break <ADDRESS> | b <ADDRESS> - set breakpoint at address\n");
            printf("  remove | rm - remove current breakpoint\n");
            printf("  regs | r - print registers\n");
            printf("  quit | q - exit debugger\n");
            printf("  help | h - show this help message\n");
            continue;
        } else {
            printf("Unknown command\n");
            continue;
        }
            waitpid(child_pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Child exited\n");
            break;
        }
    }

}
