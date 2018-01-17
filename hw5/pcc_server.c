//
// Created by gal on 1/12/18.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>

#define NUM_OF_COUNTABLE_CHARS 95

typedef struct{
    volatile unsigned int printable_chars_counts[NUM_OF_COUNTABLE_CHARS];
} pcc_total;

// global vars
static pcc_total counts;
static int socket_fd = 0;
static bool continue_accept_sockets;
static int num_of_threads;
pthread_mutex_t lock;

// when change the option to get more packers and return global counts
void my_handle(int signal){
    printf("got to handler!\n");
    continue_accept_sockets = false;
    close(socket_fd);
}

// lock
void my_lock(){
    if (pthread_mutex_lock(&lock) != 0){
        printf("can't lock\n");
        exit(-1);
    }
}

// unlock
void my_unlock(){
    if (pthread_mutex_unlock(&lock) != 0 ) {
        printf("can't unlock");
        exit(-1);
    }
}


// update local counts
void update_counts(pcc_total* my_pcc, char* string, int str_len){
    int count = 0;

    for (int i=0; i<str_len; i++){
        if (string[i] >= (char)(32) && string[i] < (char)(32 + NUM_OF_COUNTABLE_CHARS)){
            count += 1;
            my_pcc->printable_chars_counts[(int)(string[i]) - 32]++;
        }
    }
}

// update the global counters from local structures
void update_global_counter(pcc_total* my_pcc, pcc_total* global){
    for (int i=0; i<NUM_OF_COUNTABLE_CHARS; i++){
        global->printable_chars_counts[i] += my_pcc->printable_chars_counts[i];
    }
}

// return sum of counter in counter structure
int count_printable(pcc_total* my_pcc){
    int counter = 0;
    for (int i=0; i<NUM_OF_COUNTABLE_CHARS; i++){
        counter += my_pcc->printable_chars_counts[i];
    }
    return counter;
}

void* thread_action(void* params){
    // init local params
    int sock_id = (intptr_t) params;
    pcc_total local_counters;
    for (int i=0; i < NUM_OF_COUNTABLE_CHARS; i++){
        local_counters.printable_chars_counts[i] = 0;
    }

    char recv_buff[1024];
    memset(recv_buff, 0, 1024);
    int data_from_client = read(sock_id, recv_buff, 1024);

    // update printable counts structure
    // reading from client...
    while (data_from_client > 0){
        update_counts(&local_counters, recv_buff, data_from_client);
        data_from_client = read(sock_id, recv_buff, 1024);
    }

    if (data_from_client < 0){
        printf("\n Error : Didn't get data from client properly. %s \n", strerror(errno));
        exit(-1);
    }


    // send back to client num of printable
    int counter = count_printable(&local_counters);

    int suc = write(sock_id, &counter, sizeof(unsigned int));
    int tmp_written_bytes = suc;


    // write back chars
    while (tmp_written_bytes < sizeof(unsigned int)){
        suc = write(sock_id, ((char*) &counter) + tmp_written_bytes, sizeof(unsigned int));
        tmp_written_bytes += suc;
    }

    // finilize -- close sockets!
    close(sock_id);
    my_lock();
    update_global_counter(&local_counters, &counts);
    num_of_threads -= 1;
    my_unlock();
    return NULL;
}


int main(int argc, char *argv[])
{
    if (argc != 2){
        printf("port is required!\n");
    }

    // initialize variables
    int port = atoi(argv[1]);
    continue_accept_sockets = true;

    for (int i=0; i < NUM_OF_COUNTABLE_CHARS; i++){
        counts.printable_chars_counts[i] = 0;
    }

    // define vars for connection!
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset( &serv_addr, 0, addrsize );

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    //  Copied from recitation 10
    // bind
    if( 0 != bind( socket_fd, (struct sockaddr*) &serv_addr, addrsize ) )
    {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        exit (-1);
    }

    // listen
    if( 0 != listen( socket_fd, 10 ) )
    {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        exit(-1);
    }

    // take care of signal
    signal(SIGINT, my_handle);


    while( continue_accept_sockets )
    {
        // accept
        int connfd = accept( socket_fd, NULL, NULL);

        // check if connfd works
        if( connfd < 0 )
        {
            // it's OK!
            if (continue_accept_sockets == false){
                break;
            }

            // Error occurred
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            exit(-1);
        }

        printf("socket accepted!\n");

        num_of_threads += 1;
        pthread_t new_thread;
        if (pthread_create(&new_thread, NULL, thread_action, (void*) (intptr_t)connfd) != 0){
            printf("%s\n",strerror(errno));
            exit(-1);
        }
    }

    while (num_of_threads > 0){
        // wait for thread to finish....
    }

    // print total_count
    for (int i=0; i<NUM_OF_COUNTABLE_CHARS; i++){
        printf("char '%c' : %u times \n", (char)(32 + i), counts.printable_chars_counts[i]);
    }

    // server finished well
    pthread_mutex_destroy(&lock);
    return 0;
}
