
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
#include <stdbool.h>
#include <sys/types.h>

extern pid_t child_pid;
extern bool breakpoint_active;
extern long breakpoint_addr;
extern unsigned char breakpoint_orig_byte;
extern bool breakpoint_is_set;