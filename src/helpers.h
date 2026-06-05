//
// Created by jay20 on 08-05-2026.
//

#ifndef HELPERS_H
#define HELPERS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "helpers.h"
#include <stdbool.h>

const char *builtin_commands[] = {"exit", "echo", "type", "pwd", "cd","jobs"};

void execute_arg(char* cmd, char* args[]);

void trim(char *str) {
    int start = 0;
    while (str[start] == ' ') start++;
    memmove(str, str + start, strlen(str) - start + 1);
}
void trim_sted(char *str) {
    int start = 0;
    while (str[start] == ' ') start++;
    memmove(str, str + start, strlen(str) - start + 1);

    int end = strlen(str) - 1;
    while (end >= 0 && str[end] == ' ') str[end--] = '\0';
}

int find_in_path(const char *cmd) {
    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        printf("PATH is null\n");
        return 0;
    }
    char path[2000];
    strcpy(path,path_env);

    char *dir = path;
    char *colon;
    while (dir!=NULL) {
        colon = strchr(dir,':');
        if (colon) *colon = '\0';
        char fullpath[1000];
        strcpy(fullpath, dir);
        strcat(fullpath,"/");
        strcat(fullpath,cmd);

        if (access(fullpath,X_OK)==0) {
            if (colon) *colon = ':';
            printf("%s is %s\n", cmd, fullpath);
            return 1;
        }
        if (colon) {
            *colon = ':';
            dir = colon+1;
        }
        else break;
    }
    printf("%s: not found\n", cmd);
    return 0;
}

int find_in_path_without_printing(const char *cmd) {
    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        printf("PATH is null\n");
        return 0;
    }
    char path[2000];
    strcpy(path,path_env);
    char *dir = path;
    char *colon;
    while (dir!=NULL) {
        colon = strchr(dir,':');
        if (colon) *colon = '\0';
        char fullpath[1000];
        strcpy(fullpath, dir);
        strcat(fullpath,"/");
        strcat(fullpath,cmd);

        if (access(fullpath,X_OK)==0) {
            if (colon) *colon = ':';
            return 1;
        }
        if (colon) {
            *colon = ':';
            dir = colon+1;
        }
        else break;
    }
    return 0;
}
bool is_builtin_command(const char *command) {
    for (int i = 0; i < sizeof(builtin_commands) / sizeof(builtin_commands[0]); ++i) {
        if (strcmp(command, builtin_commands[i]) == 0) {
            return true;
        }
         }
    return false;
}

void parse_input(char *input, char **args) {
    int argc = 0;
    char *p = input;
    char buffer[1024];

    while (*p!='\0' && *p != '\n') {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n') break;

        int buf_len = 0;

        while (*p != '\0' && *p != '\n' && (*p != ' ' && *p != '\t')) {
            if (*p == '\'') {
                p++;
                while (*p != '\0' && *p != '\'') {
                    buffer[buf_len++] = *p++;
                }
                if (*p == '\'') p++;
            } else if (*p == '"') {
                p++;
                while (*p != '\0' && *p != '"') {
                    if (*p == '\\' && (*(p+1) == '"' || *(p+1) == '\\')) {
                        p++;
                        buffer[buf_len++] = *p++;
                    } else {
                        buffer[buf_len++] = *p++;
                    }
                }
                if (*p == '"') p++;
            }
            else if (*p == '\\') {
                p++;
                if (*p != '\0' && *p != '\n') {
                    buffer[buf_len++] = *p++;
                }
            }
            else {
                buffer[buf_len++] = *p++;
            }
        }
        buffer[buf_len] = '\0';
        args[argc++] = strdup(buffer);
    }
    args[argc] = NULL;
}

void builtin_echo(char* args[]){
    int i = 1;
    while (args[i]!=NULL) {
        printf("%s ", args[i]);
        i++;
    }
    printf("\n");
}
void builtin_pwd() {
    printf("%s\n", getcwd(NULL, 0));
}
void builtin_execute_executable(char* cmd, char* args[]) {
    if (access(cmd, X_OK) == 0) { //X_OK checkes if its an executable
        execute_arg(cmd, args);
    } else {
        printf("%s: command not found\n", cmd);
    }
}
void builtin_type(char* args[]) {
    int i_ptr = 1;
    while (args[i_ptr]!=NULL) {
        if (is_builtin_command(args[i_ptr]) == true)  {
            printf("%s is a shell builtin\n", args[i_ptr]);
        }
        else {
            int x = find_in_path(args[i_ptr]);
        }
        i_ptr++;
    }
}
void builtin_cd(char* args[]) {
    if (args[1] == NULL || strcmp(args[1], "~") == 0) {
        char *home = getenv("HOME");
        if (home != NULL) {
        chdir(home);
        } else {
        printf("cd: HOME environment variable not set\n");
            }
        }
    else if (args[2]!=NULL) {
            printf("Too many arguments\n");
        }
    else {
        if (chdir(args[1])!=0) {
                printf("cd: %s: No such file or directory\n", args[1]);
            }
        }
}
void execute_arg(char* cmd, char* args[]) {
    if (strcmp(cmd, "echo") == 0) {
        builtin_echo(args);
        return;
    }
    if (strcmp(cmd, "pwd") == 0) {
        builtin_pwd();
        return;
    }
    if (strcmp(cmd, "type") == 0) {
        builtin_type(args);
        return;
    }
    if (strcmp(cmd, "cd") == 0) {
        builtin_cd(args);
        return;
    }
    if (strcmp(cmd, "jobs") == 0) {
        return;
    }
    execvp(cmd, args);
    printf("%s cmd not found\n", cmd);
    exit(1);
}
#endif //HELPERS_H