#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

#define MAX_JOBS 100

typedef struct {
    pid_t pid;
    char cmd[1024];
    bool active;
} Job;

extern Job jobs_list[MAX_JOBS];
extern const char *builtin_commands[];

void execute_arg(char* cmd, char* args[]);
void trim(char *str);
void trim_sted(char *str);
int find_in_path(const char *cmd);
int find_in_path_without_printing(const char *cmd);
bool is_builtin_command(const char *command);
void parse_input(char *input, char **args);
void builtin_echo(char* args[]);
void builtin_pwd(void);
void builtin_execute_executable(char* cmd, char* args[]);
void builtin_type(char* args[]);
void builtin_cd(char* args[]);
void builtin_jobs(void);
#endif
