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



void run_cli(const char *program, char **args);