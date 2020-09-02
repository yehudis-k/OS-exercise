#include "myshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

struct sigaction sa;

int prepare(void)
{
    sa.sa_handler=SIG_IGN;
    sigaction(SIGINT, &sa, NULL);
    printf("\nMy command > ");
    return 0;
}

int process_arglist(int count, char **arglist) {
    int is_pipe = 0;
    int stat_loc;
    int pipefd[2];
    int pipe_index;

    //first case: There's an ampersand - don't wait
    if (!strcmp(arglist[count - 1], "&")) {
        arglist[count - 1] = NULL;
        pid_t child_pid = fork();
        if (child_pid == -1) {
            fprintf(stderr, "\nFailed forking Child");
            exit(1);
        }
        if (child_pid == 0) {
            if (execvp(arglist[0], arglist) < 0) {
                fprintf(stderr, "\nCould not exec command");
                exit(1);
            }
        }
        if (child_pid>0) {
            signal(SIGCHLD,SIG_IGN);
        }
        printf("\nMy command > ");
        return 1;
    }

    //Check if there's pipe
    for (int j = 0; j < count; j++) {
        if (strcmp(arglist[j], "|") == 0) {
            is_pipe = 1;
            pipe_index = j;
            if (pipe(pipefd) < 0) {
                fprintf(stderr, "\nFailed pipe");
                exit(1);
            }
            break;
        }
    }

    //Second case: There's pipe
    if (is_pipe == 1) {
        arglist[pipe_index] = NULL;
        pid_t child_pid = fork();//create first child
        if (child_pid == -1) {
            fprintf(stderr, "\nFailed forking Child");
            exit(1);
        }
        else if (child_pid == 0) {
            sa.sa_handler=SIG_DFL;
            sigaction(SIGINT, &sa, NULL);
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            if (execvp(arglist[0], arglist) < 0) {
                fprintf(stderr, "\nCould not exec command");
                exit(1);
            }
        }
        else if (child_pid>0){
            pid_t child_pid2=fork();//create second child
            if (child_pid2 == -1) {
                fprintf(stderr, "\nFailed forking Child");
                exit(1);
            }
            else if (child_pid2==0) {
                sa.sa_handler=SIG_DFL;
                sigaction(SIGINT, &sa, NULL);
                close(pipefd[1]);
                dup2(pipefd[0],STDIN_FILENO);
                close(pipefd[0]);
                if (execvp(arglist[pipe_index+1], arglist+pipe_index+1) < 0) {
                    fprintf(stderr, "\nCould not exec command");
                    exit(1);
                    }

                }
            else if (child_pid2>0) {
                close(pipefd[1]);
                close(pipefd[0]);
                waitpid(child_pid, &stat_loc, 0);
                waitpid(child_pid2, &stat_loc, 0);
            }
        }
    }


    //Third case: the rest of the cases - no pipe no &
    else {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            fprintf(stderr, "\nFailed forking Child");
            exit(1);
        } else if (child_pid == 0) {
            sa.sa_handler=SIG_DFL;
            sigaction(SIGINT, &sa, NULL);
            if (execvp(arglist[0], arglist) < 0) {
                fprintf(stderr, "\nCould not exec command");
                exit(1);
            }
        }
        else {
            waitpid(child_pid, &stat_loc, 0);
        }
    }
    printf("\nMy command > ");
    return 1;
}



int finalize(void)
{
    printf("\n-------Goodbye!!!-------\n");
    return 0;
}
