#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


// locates the pipe index if exists
int locate_pipe(int count, char ** arglist){
    for (int i=0; i<count; i++){
        if (strcmp(arglist[i], "|") == 0){
            return i;
        }
    }
    return -1;
}

//checks if there is an ampercent in the commands
bool has_ampercent(int count, char** arglist){
    if (strcmp(arglist[count-1], "&") == 0){
        return true;
    }
    return false;
}

// run child command in case it's a legal one, if not, returns 0 - this function takes care only in case there is not |
void take_care_of_single_child(char** arglist){
    if (execvp(arglist[0], arglist) == -1){
        fprintf(stderr,"execvp failed\n\0",15);
        exit(1);
    }
}

int process_arglist(int count, char** arglist){

    struct sigaction ignore_signal;
    memset(&ignore_signal, 0, sizeof(ignore_signal));
    ignore_signal.sa_handler = SIG_IGN;
    struct sigaction default_signal;
    memset(&default_signal, 0, sizeof(default_signal));
    default_signal.sa_handler = SIG_DFL;

    //ignore_signal.sa_sigaction = signal_ignore_handler;
    if (sigaction(SIGINT, &ignore_signal, NULL) != 0){
        fprintf(stderr,"error initializing signal\n\0",27);
        exit(1);
    }

    bool run_in_background = false;
    int pipe_index = -1;
    // checks if there is positive amount of commands, if not, exit
    if (count <= 0){
        fprintf(stderr,"non positives amount of commands!\n",30);
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
            fprintf(stderr,"error in pipe!\n\0",16);
            exit(1);
        }
    }

    pid_t pid = fork();

    // we are in child process
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
                fprintf(stderr,"error in dup!\n\0",15);
                exit(1);
            }
            close(pipe_fd[1]);
            arglist[pipe_index] = NULL;
            take_care_of_single_child(arglist);
        }
    }


    pid_t pid_2;

    if (pipe_index != -1) {

        pid_2 = fork();

        if (pid_2 == 0) {
            sigaction(SIGINT, &default_signal, NULL);
            close(pipe_fd[1]);
            if (dup2(pipe_fd[0], 0) == -1){
                fprintf(stderr,"error in dup!\n\0",15);
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
        if ((waitpid(pid, &status, 0) == pid) && (waitpid(pid_2, &status2, 0) == pid_2)){
            return 1;
        }
    }
    return 1;
}

// prepare and finalize calls for initialization and destruction of anything required

int prepare(void){
    struct sigaction child_signal;
    memset(&child_signal, 0, sizeof(child_signal));
    child_signal.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &child_signal, NULL);
    return 0;
}


int finalize(void){
    return 0;
}

int main(int argc,char** argv){
    prepare();

    char* arglist0[] = {"ls", "-la", "|", "grep", "-i", "hw", NULL};
    process_arglist(6, arglist0);

    char* arglist2[] = {"sleep", "10", "&", NULL};
    process_arglist(3, arglist2);

    char* arglist[] = {"sleep", "60", NULL};
    process_arglist(2, arglist);

    finalize();
}

