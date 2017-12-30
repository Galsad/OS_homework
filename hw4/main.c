#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>

#define BUFFER_SIZE (1<<20)

// GLOBAL VARIABLES //
char common_buffer [BUFFER_SIZE] = {0};
int num_of_done_jobs = 0;

pthread_mutex_t lock;
pthread_cond_t lock2;

int number_of_writers;
int output_fd = 0;
int chunk = 0;
int rc; // checking whether write failed!
int length_to_write;

// XORing a block to the common block
void XOR_blocks(char* block, int block_size){
    printf("in XOR blocks!\n");
    int i=0;
    for (i=0; i < block_size; i++){
        common_buffer[i] ^= block[i];
    }
}

// set common blocks to zeros
void reset_common_block(){
    int i = 0;
    for (i=0; i < BUFFER_SIZE; i++){
        common_buffer[i] = 0;
    }
}


int read_next_chunk(int fd, char* tmp_buffer){
    int last_bytes_read = 0;
    // reading to buffer
    rc = read(fd, tmp_buffer, BUFFER_SIZE);
    last_bytes_read = rc;

    while (last_bytes_read < BUFFER_SIZE && rc > 0){
        rc = read(fd, tmp_buffer, BUFFER_SIZE);
        last_bytes_read += rc;
    }
    return last_bytes_read;
}


void* worker_job(void* t){
    char tmp_buffer [BUFFER_SIZE];
    char* file_path = (char*) t;
    int fd = open(file_path, O_RDONLY);
    int current_chunk = 0;
    int last_bytes_read;

    if (fd < 0){
        //TODO - print error
        printf("cannot open file");
        exit(1);
    }

    while ( (last_bytes_read = read_next_chunk(fd, tmp_buffer)) > 0){
        if (pthread_mutex_lock(&lock) != 0){
            printf("can't lock\n");
            exit(2);
        }

        if (current_chunk != chunk){
            if (pthread_cond_wait(&lock2, &lock) != 0 ){
                printf("error in lock2!");
                exit(2);
            }
        }

        // XORing the data
        XOR_blocks(tmp_buffer, last_bytes_read);
        if (pthread_mutex_unlock(&lock) != 0 ) {
            printf("can't unlock");
            exit(2);
        }

        if (pthread_mutex_lock(&lock) != 0){
            printf("can't lock\n");
            exit(2);
        }

        num_of_done_jobs += 1;
        // if you are the last job - write to output
        if (num_of_done_jobs == number_of_writers){
            //TODO - fix buffer size to the actual length
            rc = write(output_fd, common_buffer, last_bytes_read);
            if (rc < 0){
                printf("couldn't write to file!");
                exit(2);
            }
            // reset the buffer
            reset_common_block();
            num_of_done_jobs = 0;
            chunk += 1;

            rc = pthread_cond_broadcast(&lock2);
            if (rc < 0){
                printf("can't unlock 2!");
                exit(2);
            }
        }

        // unlock !
        if (pthread_mutex_unlock(&lock) != 0 ) {
            printf("can't unlock");
            exit(2);
        }

        current_chunk += 1;

    } // end of while

    // lock
    if (pthread_mutex_lock(&lock) != 0){
        printf("can't lock\n");
        exit(2);
    }

    if (current_chunk != chunk){
        if (pthread_cond_wait(&lock2, &lock) != 0 ){
            printf("error in lock2!");
            exit(2);
        }
    }

    if (num_of_done_jobs != number_of_writers - 1){
        number_of_writers -= 1;
        if (pthread_mutex_unlock(&lock) != 0){
            printf("can't lock\n");
            exit(2);
        }
    }

    else{
        number_of_writers -= 1;
        rc = write(output_fd, common_buffer, last_bytes_read);
        if (rc < 0){
            printf("still can't write...\n");
            exit(2);
        }
        reset_common_block();
        num_of_done_jobs = 0;
        chunk += 1;

        rc = pthread_cond_broadcast(&lock2);
        if (rc < 0){
            printf("can't unlock 2!");
            exit(2);
        }

        if (pthread_mutex_unlock(&lock) != 0){
            printf("can't lock\n");
            exit(2);
        }
    }


}

//void* busy_work(void* t){
//    printf("in busy work!\n");
//    struct thread_info* tinfo = t;
//    char tmp_buffer [BUFFER_SIZE];
//    int file_pointer = 0;
//
//
//    // if thread finished reading his file
//    // TODO -- add lock on the inc
//    if (tinfo->is_done == true){
//        num_of_done_jobs += 1;
//        printf("num of jobs is %d \n", num_of_done_jobs);
//    }
//
//    // TODO -- add lock here and check errors
//    else{
//        printf("have some work to do!\n");
//        file_pointer = read(tinfo->fid, tmp_buffer, BUFFER_SIZE);
//        // TODO -- need to make sure that amount of read bytes is set to 0 after every time!
//        tinfo->amount_of_read_bytes = tinfo->amount_of_read_bytes + file_pointer;
//        printf("num of read bytes are: %d\n", tinfo->amount_of_read_bytes);
//
//        pthread_mutex_lock(&lock);
//
//        XOR_blocks(tmp_buffer, tinfo->amount_of_read_bytes);
//        num_of_done_jobs += 1;
//        // if whole file was read set is_done to true
//        if (tinfo->amount_of_read_bytes == tinfo->file_length){
//            tinfo->is_done = true;
//        }
//    }
//
//    // all thread are done - write the data to the output file and set num_of_jobs_done to 0
//    printf("num of jobs is %d \n", num_of_done_jobs);
//    if (num_of_done_jobs == tinfo->num_of_threads){
//        printf("reseting... \n");
//        num_of_done_jobs = 0;
//        // TODO - check if write worked and check if need to write less in last block
//        write(tinfo->output_fid, common_buffer, BUFFER_SIZE);
//        reset_common_block();
//        pthread_mutex_unlock(&lock2);
//    }
//    sleep(1);
//    pthread_mutex_unlock(&lock);
//}

//// checks if all the files are done writing their content to output
//bool all_done(struct thread_info **tinfo, int lst_size){
//    for (int i=0; i<lst_size; i++){
//        if (tinfo[i]->is_done == false){
//            return false;
//        }
//    }
//    return true;
//}


int main(int argc, char** argv) {
    if (argc <= 2){
        printf("input must be with size 3 or more\n");
        exit (-1);
    }

    output_fd = open(argv[1], O_WRONLY | O_TRUNC);
    printf("Hello, creating %s from %d input files\n", argv[1], argc - 2);

    // initialize variables for the program
    number_of_writers = argc - 2;
    pthread_t* thread_ids = malloc(number_of_writers * sizeof(pthread_t));

    size_t i = 0;
    for (i=0; i<number_of_writers; i++){
        pthread_create(&thread_ids[i], NULL, worker_job, argv[i+2]);
    }

    // wait for all threads to end
    for (i=0; i<number_of_writers; i++){
        pthread_join(thread_ids[i], NULL);
    }

    // close output file
    close(output_fd);

    // free all variables!
    free(thread_ids);

    printf("I am all good!\n");

    return 0;
}