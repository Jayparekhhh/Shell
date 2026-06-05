#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "helpers.h"
#include <stdbool.h>
#include <fcntl.h>
void trim(char *str);
void trim_sted(char *str);
int find_in_path(const char *cmd);
int find_in_path_without_printing(const char *cmd);
void execute_arg(char* cmd, char* args[]);
bool is_builtin_command(const char *command);
void parse_input(char *input, char **args);
void builtin_echo(char* args[]);
void builtin_pwd();
void builtin_execute_executable(char* cmd, char* args[]);
void builtin_type(char* args[]);
void builtin_cd(char* args[]);

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    int job_id = 0;
    char *args[50];
    for (int i = 0; i < 50; i++) args[i] = NULL;
    while (1) {
        while (waitpid(-1, NULL, WNOHANG) > 0);

        for (int i = 0; i < 50; i++) {
            if (args[i] != NULL) {
                free(args[i]);
                args[i] = NULL;
            }
        }
        printf("$ ");
        char input[1000];

        if(fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        trim(input); //remove front whitespace

        input[strcspn(input, "\n")] = '\0';
        bool background = false;
        parse_input(input, args);

        for (int i = 0; args[i] != NULL; i++) {
            if (args[i+1] == NULL && strcmp(args[i], "&") == 0) {
                free(args[i]);
                args[i] = NULL;
                background = true;
                break;
            }
        }
        char *cmd = args[0];
        // command and arguments like gcc -o blah blah, gcc cmd rest goes in args
        bool redirect = false;
        bool append = false;
        char *redirect_file = NULL;

        for(int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], ">") == 0 || strcmp(args[i], "1>") == 0) {
                redirect = true;
                append = false;
                redirect_file = args[i+1];
                args[i] = NULL;
                break;
            } else if (strcmp(args[i], ">>") == 0 || strcmp(args[i], "1>>") == 0) {
                redirect = true;
                append = true;
                redirect_file = args[i+1];
                args[i] = NULL;
                break;
            }
        }
        if (cmd == NULL) continue;
        args[0] = cmd;
        char *arg = args[1];

        bool is_builtin = is_builtin_command(cmd);
        if (background || !is_builtin) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                continue;
            }
            if (pid > 0) {
                if (background) {
                    printf("[%d] %d\n", ++job_id, pid);
                } else {
                    waitpid(pid, NULL, 0);
                }
                continue;
            }
        }


        int saved_stdout = dup(STDOUT_FILENO);
        if (redirect && redirect_file != NULL) {
            int flags = O_WRONLY | O_CREAT;
            if (append) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            int fd = open(redirect_file, flags, 0644);
            if (fd < 0) {
                perror("open");
                close(saved_stdout);
                if (background || !is_builtin) exit(1);
                else continue;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (strcmp(cmd, "exit")==0) {
            if (redirect) {
                fflush(stdout);
                dup2(saved_stdout, STDOUT_FILENO);
            }
            close(saved_stdout);
            break;
        }
        else{
            execute_arg(cmd, args);
        }
        if (redirect) {
            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
        }
        close(saved_stdout);
        if (background) {
            exit(0);
        }
    }
    return 0;
}