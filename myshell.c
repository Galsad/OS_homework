#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>


// locates the pipe index if exists
int locate_pipe(int count, char ** arglist){
    for (int i=0; i<count; i++){
        if (strcmp(arglist[i], "|") == 0){
            return i;
        }
    }
    return -1;
}

//checks if one of the commands is ampercent or not!
bool has_ampercent(int count, char** arglist){
    if (strcmp(arglist[count-1], "&") == 0){
        return true;
    }
    return false;
}

// run child command in case it's a legal one, if not, returns 0 - this function takes care only in case there is not |
void take_care_of_single_child(char** arglist){
    if (execvp(arglist[0], arglist) == -1){
        fprintf(stderr,"execvp failed\n");
        exit(1);
    }
}

int process_arglist(int count, char** arglist){

    struct sigaction ignore_sig;
    memset(&ignore_sig, 0, sizeof(ignore_sig));
    ignore_sig.sa_handler = SIG_IGN;
    struct sigaction default_signal;
    memset(&default_signal, 0, sizeof(default_signal));
    default_signal.sa_handler = SIG_DFL;

    //ignore_signal.sa_sigaction = signal_ignore_handler;
    if (sigaction(SIGINT, &ignore_sig, NULL) != 0){
        fprintf(stderr,"error initializing signal\n");
        exit(1);
    }

    bool run_in_background = false;
    int pipe_index = -1;
    // checks if there is positive amount of commands, if not, exit
    if (count <= 0){
        fprintf(stderr,"need to enter commands!\n");
        return 1;
    }

    // checks whether there is an & or nor
    run_in_background = has_ampercent(count, arglist);

    // find pipe
    pipe_index = locate_pipe(count, arglist);
    printf("%d\n", pipe_index);



    int pipe_fd[2] = {0};
    if (pipe_index != -1){
        if (pipe(pipe_fd)){
            fprintf(stderr,"error in pipe creation!\n");
            exit(1);
        }
    }

    pid_t pid = fork();

    // we are in first child process
    if (pid == 0) {
        if (pipe_index == -1) {
            // ignore the last argument
            if (run_in_background == true) {
                //sigaction(SIGINT, &ignore_signal, NULL);
                arglist[count - 1] = NULL;
                take_care_of_single_child(arglist);
            }
                // don't ignore the last argument
            else {
                sigaction(SIGINT, &default_signal, NULL);
                take_care_of_single_child(arglist);
            }
        }

        else{
            sigaction(SIGINT, &default_signal, NULL);
            close(pipe_fd[0]);
            if (dup2(pipe_fd[1], 1) == -1){
                fprintf(stderr,"error in dupilcate!\n");
                exit(1);
            }
            close(pipe_fd[1]);
            arglist[pipe_index] = NULL;
            take_care_of_single_child(arglist);
        }
    }

    // define pid for the second child!
    pid_t pid2;

    if (pipe_index != -1) {

        pid2 = fork();
        if (pid2 == 0) {
            sigaction(SIGINT, &default_signal, NULL);
            close(pipe_fd[1]);
            if (dup2(pipe_fd[0], 0) == -1){
                fprintf(stderr,"error in duplicate!\n");
                exit(1);
            }
            close(pipe_fd[0]);
            take_care_of_single_child(&(arglist[pipe_index + 1]));
        }

        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }

    if (run_in_background){
        return 1;
    }

    int status = 0;
    int status2 = 0;
    if (pipe_index == -1) {
        if (waitpid(pid, &status, 0) == pid) {
            return 1;
        }
    }

    else{
        if ((waitpid(pid, &status, 0) == pid) && (waitpid(pid2, &status2, 0) == pid2)){
            return 1;
        }
    }
    return 1;
}


int prepare(void){
    struct sigaction son_signal;
    memset(&son_signal, 0, sizeof(son_signal));
    son_signal.sa_handler = SIG_IGN;

    // check if sigaction failed
    if (sigaction(SIGCHLD, &son_signal, NULL) == -1){
        fprintf(stderr,"sigaction failed!\n");
        exit(1);
    }
    return 0;
}


int finalize(void){
    // nothing to do in the end of the program - no need to free anything!
    return 0;
}


