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

        char **cmds[50]; 
        int cmd_count = 0;
        cmds[cmd_count++] = &args[0];

        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "|") == 0) {
                args[i] = NULL; 
                cmds[cmd_count++] = &args[i + 1]; 
            }
        }

        if (cmd_count > 1) {
            int in_fd = STDIN_FILENO;
            int pipefd[2];
            pid_t pids[50];
            pid_t last_pid = -1;

            for (int i = 0; i < cmd_count; i++) {
                if (cmds[i][0] == NULL) continue; 

                if (i < cmd_count - 1) {
                    if (pipe(pipefd) == -1) {
                        perror("pipe");
                        break;
                    }
                }

                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    break;
                }

                if (pid == 0) {
                    if (in_fd != STDIN_FILENO) {
                        dup2(in_fd, STDIN_FILENO);
                        close(in_fd);
                    }
                    if (i < cmd_count - 1) {
                        close(pipefd[0]);
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);
                    }

                    if (is_builtin_command(cmds[i][0])) {
                        execute_arg(cmds[i][0], cmds[i]);
                        exit(0);
                    } else {
                        execvp(cmds[i][0], cmds[i]);
                        printf("%s cmd not found\n", cmds[i][0]);
                        exit(1);
                    }
                }

                if (in_fd != STDIN_FILENO) {
                    close(in_fd);
                }
                if (i < cmd_count - 1) {
                    close(pipefd[1]); 
                    in_fd = pipefd[0];
                }
                pids[i] = pid;
                last_pid = pid;
            }

            if (background) {
                int assigned_job_id = -1;
                for (int j = 0; j < MAX_JOBS; j++) {
                    if (!jobs_list[j].active) {
                        jobs_list[j].active = true;
                        jobs_list[j].pid = last_pid;
                        strcpy(jobs_list[j].cmd, cmd_copy);
                        assigned_job_id = j + 1;
                        break;
                    }
                }
                if (assigned_job_id != -1) printf("[%d] %d\n", assigned_job_id, last_pid);
            } else {
                for (int i = 0; i < cmd_count; i++) {
                    waitpid(pids[i], NULL, 0);
                }
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
            if (append) flags |= O_APPEND; else flags |= O_TRUNC;
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
