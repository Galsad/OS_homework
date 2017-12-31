#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE (1<<20)

// GLOBAL VARIABLES //
char common_buffer [BUFFER_SIZE] = {0};
int num_of_done_jobs = 0;

pthread_mutex_t lock;
pthread_cond_t lock2;

int number_of_writers; // number of inputs
int output_fd = 0; // fd of output file
int chunk = 0; // number of current chunk
int rc; // checking whether write failed!
int max_block; //maximum size to write to a block
int output_length = 0;

void lock_1(){
    if (pthread_mutex_lock(&lock) != 0){
        printf("can't lock\n");
        exit(3);
    }
}

void unlock_1(){
    if (pthread_mutex_unlock(&lock) != 0 ) {
        printf("can't unlock");
        exit(3);
    }
}

// XORing a block to the common block
void XOR_blocks(char* block, int block_size){
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

// read next buffer to a chunk
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
    // init thread vars
    char tmp_buffer [BUFFER_SIZE];
    char* file_path = (char*) t;
    int fd = open(file_path, O_RDONLY);
    int current_chunk = 0;
    int last_bytes_read;

    if (fd < 0){
        printf("cannot open file");
        exit(1);
    }

    while ( (last_bytes_read = read_next_chunk(fd, tmp_buffer)) > 0){
        lock_1();

        if (current_chunk != chunk){
            if (pthread_cond_wait(&lock2, &lock) != 0 ){
                printf("error in lock2!");
                exit(2);
            }
        }

        // XORing the data
        XOR_blocks(tmp_buffer, last_bytes_read);

        // inc the number of done jobs
        num_of_done_jobs += 1;
        if (last_bytes_read > max_block){
            max_block = last_bytes_read;
        }

        // if you are the last job - write to output
        if (num_of_done_jobs == number_of_writers){
            rc = write(output_fd, common_buffer, max_block);
            if (rc < 0){
                printf("couldn't write to file!");
                exit(2);
            }

            // reset the buffer and parameters!
            reset_common_block();
            num_of_done_jobs = 0;
            chunk += 1;
            output_length += max_block;
            max_block = 0;

            rc = pthread_cond_broadcast(&lock2);
            if (rc < 0){
                printf("can't unlock 2!");
                exit(2);
            }
        }
        unlock_1();
        current_chunk += 1;

    } // end of while

    lock_1();

    if (current_chunk != chunk){
        if (pthread_cond_wait(&lock2, &lock) != 0 ){
            printf("error in lock2!");
            exit(2);
        }
    }

    if (num_of_done_jobs != number_of_writers - 1){
        number_of_writers -= 1;
        unlock_1();
    }

    else{
        number_of_writers -= 1;
        rc = write(output_fd, common_buffer, max_block);
        if (rc < 0){
            printf("still can't write...\n");
            exit(2);
        }
        reset_common_block();
        num_of_done_jobs = 0;
        chunk += 1;
        max_block = 0;

        rc = pthread_cond_broadcast(&lock2);
        if (rc < 0){
            printf("can't unlock 2!");
            exit(2);
        }
        unlock_1();
    }
    pthread_exit( (void*) t );
}

int main(int argc, char** argv) {
    if (argc <= 2){
        printf("input must be with size 3 or more\n");
        exit (-1);
    }

    // open output and print welcome message
    output_fd = open(argv[1], O_WRONLY | O_TRUNC);
    if (output_fd < 0){
        printf("cannot open output file!");
        exit(4);
    }
    printf("Hello, creating %s from %d input files\n", argv[1], argc - 2);

    // initialize variables for the program
    number_of_writers = argc - 2;
    pthread_t* thread_ids = malloc(number_of_writers * sizeof(pthread_t));

    // create thread for each input file
    size_t i = 0;
    for (i=0; i<number_of_writers; i++){
        pthread_create(&thread_ids[i], NULL, worker_job, argv[i+2]);
    }

    // wait for all threads to end
    for (i=0; i<number_of_writers; i++){
        pthread_join(thread_ids[i], NULL);
    }

    //destroy locks
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&lock2);

    // close output file
    close(output_fd);

    printf("Created %s with size %d bytes\n", argv[1], output_length);

    // free all variables!
    free(thread_ids);

    return 0;
}