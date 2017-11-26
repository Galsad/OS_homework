#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


// TODO - errors should be written to strerr, need to write exit(1)
// TODO - CTRL-D exits?
// TODO - free struct memory (finalize)
// TODO - move sig_action to prepare
// TODO - make sure 1 is always returned from process arglist
// TODO - check success of execvp, dup (and to execute exit(1)if needed)
// TODO - take care of zombies (SIGCHILD)
// TODO - prepare returns 0 on success
// TODO - skeleton calls finilize and returns 0 on success
// TODO - separate the waits in the while loops


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
        printf("child died!\n");
        exit(1);
    }
}

static void signal_ignore_handler(int signal, siginfo_t *info, void *context){
    printf("bla bla\n");
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
        printf("error initializing signal");
        return 1;
    }

    bool run_in_background = false;
    int pipe_index = -1;
    // checks if there is positive amount of commands, if not, exit
    if (count <= 0){
        printf("non positives amount of commands!\n");
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
            printf("error in pipe!");
            return 1;
        }
    }

    pid_t pid = fork();

    // we are in child process
    if (pid == 0) {
        if (pipe_index == -1) {
            // ignore the last argument
            if (run_in_background == true) {
                //sigaction(SIGINT, &ignore_signal, NULL);
                printf("child in bg %d\n", getpid());
                arglist[count - 1] = NULL;
                take_care_of_single_child(arglist);
            }
                // don't ignore the last argument
            else {
                printf("child in fg %d\n", getpid());
                sigaction(SIGINT, &default_signal, NULL);
                take_care_of_single_child(arglist);
            }
        }

        else{
            sigaction(SIGINT, &default_signal, NULL);
            close(pipe_fd[0]);
            dup2(pipe_fd[1], 1);
            close(pipe_fd[1]);
            arglist[pipe_index] = NULL;
            // TODO - wrap execvp
            execvp(arglist[0], arglist);
        }
    }


    pid_t pid_2;

    if (pipe_index != -1) {

        pid_2 = fork();

        if (pid_2 == 0) {

            printf("*** %d\n", getpid());

            sigaction(SIGINT, &default_signal, NULL);
            close(pipe_fd[1]);
            dup2(pipe_fd[0], 0);
            close(pipe_fd[0]);
            printf("%s\n", arglist[pipe_index+1]);
            // arglist[pipe_index] = NULL;
            execvp(arglist[pipe_index+1], &arglist[pipe_index+1]);
        }

        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }

    if (run_in_background){
        printf("run in background\n");
        return 1;
    }

    int status = 0;
    int status2 = 0;
    if (pipe_index == -1) {
        if (wait(&status) == pid) {
            printf("### pid: %d, pid2: %d\n", pid, pid_2);
            printf("child finished\n");

            // TODO - print child status?
            return 1;
        }
    }

    else{
        if ((waitpid(pid, &status, 0) == pid) && (waitpid(pid_2, &status2, 0) == pid_2)){
            printf("### pid: %d, pid2: %d\n", pid, pid_2);
            // TODO - print child status?
            return 1;
        }
    }
    return 1;
}

// prepare and finalize calls for initialization and destruction of anything required

int prepare(void){

}


int finalize(void){

}

int main(int argc,char** argv){
    char* arglist0[] = {"ls", "-la", "|", "grep", "-i", "hw", NULL};
    process_arglist(6, arglist0);

    char* arglist2[] = {"sleep", "70", NULL};
    process_arglist(2, arglist2);

    char* arglist[] = {"sleep", "60", "&", NULL};
    process_arglist(3, arglist);
}

