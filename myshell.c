//
// Created by gal on 11/23/17.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

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

void take_care_of_two_children(int pipe_index, char** arglist){
    int pipe_fd[2];
    arglist[pipe_index] = NULL;

    pid_t cpid;
    if (pipe(pipe_fd) == -1){
        printf("failed creating pipe!\n");
        exit(1);
    }


    printf("i am here!\n");
    cpid = fork();
    //printf("cpid is %d!\n", cpid);
    // child
    if (cpid == 0){
        // printf("Child - command is:\n");
        // TODO - can it fail?
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        int index = pipe_index + 1;
        while (arglist[index] != NULL){
            //printf("%s \n", arglist[index]);
            index ++;
        }

        if (execvp(arglist[pipe_index+1], &(arglist[pipe_index + 1])) == -1){
            // TODO - change it!
            printf("yeled efes!\n");
            exit(1);
        }
    }

    // father
    else{
        printf("Father - command is:\n");
        // TODO - can it fail?
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        sleep(5);
        if (execvp(arglist, arglist) == -1){
            // TODO - change it!
            printf("aba marbitz!\n");
            exit(1);
        }
    }
}

int process_arglist(int count, char** arglist){

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

    pid_t pid = fork();

    // we are in child process
    if (pid == 0) {
        if (pipe_index == -1) {
            // ignore the last argument
            if (run_in_background == true) {
                arglist[count - 1] = NULL;
                take_care_of_single_child(arglist);
            }
                // don't ignore the last argument
            else {
                take_care_of_single_child(arglist);
            }
        } else {
            take_care_of_two_children(pipe_index, arglist);
        }
    }
    int status;
    while (wait(&status) != -1);
    return 1;
}

// prepare and finalize calls for initialization and destruction of anything required

int prepare(void){

}


int finalize(void){

}

int main(int argc,char** argv){
    char* arglist[] = {"ls", "-la", "|", "grep", "-i", "hw", NULL};
    process_arglist(6, arglist);
}

