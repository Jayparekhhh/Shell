#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include "helpers.h"

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    char *args[50];
    
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs_list[i].active = false;
    }

    for (int i = 0; i < 50; i++) args[i] = NULL;
    while (1) {
        pid_t wpid;
        while ((wpid = waitpid(-1, NULL, WNOHANG)) > 0) {
            for (int j = 0; j < MAX_JOBS; j++) {
                if (jobs_list[j].active && jobs_list[j].pid == wpid) {
                    jobs_list[j].active = false; 
                    break;
                }
            }
        }

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

        trim(input); 
        input[strcspn(input, "\n")] = '\0';
        
        char cmd_copy[1000];
        strcpy(cmd_copy, input);

        bool background = false;
        parse_input(input, args);

        for (int i = 0; args[i] != NULL; i++) {
            if (args[i+1] == NULL && strcmp(args[i], "&") == 0) {
                free(args[i]);
                args[i] = NULL;
                background = true;
                
                char *amp_pos = strrchr(cmd_copy, '&');
                if (amp_pos) {
                    *amp_pos = '\0';
                    trim_sted(cmd_copy); 
                }
                break;
            }
        }

        if (args[0] == NULL) continue;

        int pipe_idx = -1;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "|") == 0) {
                pipe_idx = i;
                args[i] = NULL;
                break;
            }
        }

        if (pipe_idx != -1) {
            char **cmd1_args = args;
            char **cmd2_args = &args[pipe_idx + 1];

            if (cmd1_args[0] == NULL || cmd2_args[0] == NULL) {
                printf("Invalid pipe syntax\n");
                continue;
            }

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid1 = fork();
            if (pid1 < 0) {
                perror("fork");
                continue;
            }

            if (pid1 == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);

                if (is_builtin_command(cmd1_args[0])) {
                    execute_arg(cmd1_args[0], cmd1_args);
                    exit(0);
                } else {
                    execvp(cmd1_args[0], cmd1_args);
                    printf("%s cmd not found\n", cmd1_args[0]);
                    exit(1);
                }
            }

            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("fork");
                continue;
            }

            if (pid2 == 0) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);

                if (is_builtin_command(cmd2_args[0])) {
                    execute_arg(cmd2_args[0], cmd2_args);
                    exit(0);
                } else {
                    execvp(cmd2_args[0], cmd2_args);
                    printf("%s cmd not found\n", cmd2_args[0]);
                    exit(1);
                }
            }

            close(pipefd[0]);
            close(pipefd[1]);

            if (background) {
                int assigned_job_id = -1;
                for (int j = 0; j < MAX_JOBS; j++) {
                    if (!jobs_list[j].active) {
                        jobs_list[j].active = true;
                        jobs_list[j].pid = pid2;
                        strcpy(jobs_list[j].cmd, cmd_copy);
                        assigned_job_id = j + 1;
                        break;
                    }
                }
                if (assigned_job_id != -1) {
                    printf("[%d] %d\n", assigned_job_id, pid2);
                } else {
                    printf("Maximum number of background jobs reached.\n");
                }
            } else {
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
            }
            continue;
        }

        char *cmd = args[0];
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
        
        args[0] = cmd;
        bool is_builtin = is_builtin_command(cmd);
        
        if (background || !is_builtin) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                continue;
            }
            if (pid > 0) {
                if (background) {
                    int assigned_job_id = -1;
                    for (int j = 0; j < MAX_JOBS; j++) {
                        if (!jobs_list[j].active) {
                            jobs_list[j].active = true;
                            jobs_list[j].pid = pid;
                            strcpy(jobs_list[j].cmd, cmd_copy);
                            assigned_job_id = j + 1;
                            break;
                        }
                    }
                    if (assigned_job_id != -1) {
                        printf("[%d] %d\n", assigned_job_id, pid);
                    } else {
                        printf("Maximum number of background jobs reached.\n");
                    }
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

        if (strcmp(cmd, "exit") == 0) {
            if (redirect) {
                fflush(stdout);
                dup2(saved_stdout, STDOUT_FILENO);
            }
            close(saved_stdout);
            break;
        }
        else {
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
