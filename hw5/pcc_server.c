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

#define NUM_OF_COUNTABLE_CHARS 95


typedef struct{
    volatile unsigned int printable_chars_counts[NUM_OF_COUNTABLE_CHARS];
} pcc_total;

// global vars
static pcc_total counts;
//int listenfd;
static bool continue_accept_sockets;
static int num_of_threads;

// initialize pcc to 0's
//void initialize_pcc(struct pcc_total* my_pcc){
//    for (int i=0; i < NUM_OF_COUNTABLE_CHARS; i++){
//        my_pcc->printable_chars_counts[i] = 0;
//    }
//}

// TODO - add locks
void update_counts(pcc_total* my_pcc, char* string, int str_len){
    int count = 0;

    for (int i=0; i<str_len; i++){
        if (string[i] > (char)(32) && string[i] < (char)(32 + 95)){
            count += 1;
            my_pcc->printable_chars_counts[(int)(string[i]) - 32]++;
        }
    }
}

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
    printf("in thread action\n");
    int sock_id = (int) params;
    pcc_total local_counters;
    for (int i=0; i < NUM_OF_COUNTABLE_CHARS; i++){
        local_counters.printable_chars_counts[i] = 0;
    }

    printf("initializtion worked!\n");

    char recv_buff[1024];
    memset(recv_buff, 0, 1024);
    int data_from_client = read(sock_id, recv_buff, 1024);

    // update printable counts structure
    printf("reading from client...\n");
    while (data_from_client > 0){
        printf("before count update...\n");
        update_counts(&local_counters, recv_buff, data_from_client);
        printf("buffer is %s\n", recv_buff);
        data_from_client = read(sock_id, recv_buff, 1024);
        printf("data_from_client is %d \n", data_from_client);
    }

    printf("done reading from client...\n");

    if (data_from_client < 0){
        //TODO -- print the errors
        exit(-1);
    }

    printf("count printable chars...\n");

    // send back to client num of countables
    int counter = count_printable(&local_counters);

    printf("num of countables is %d\n", counter);

    int suc = write(sock_id, &counter, sizeof(unsigned int));
    int tmp_written_bytes = suc;

    printf("write back chars...\n");

    while (tmp_written_bytes < sizeof(unsigned int)){
        suc = write(sock_id, ((char*) &counter) + tmp_written_bytes, sizeof(unsigned int));
        tmp_written_bytes += suc;
    }

    printf("DONE!\n");

    // finilize -- close sockets!
    close(sock_id);
    __sync_synchronize(); // TODO -- change it to lock
    update_global_counter(&local_counters, &counts);
    __sync_synchronize();
    __sync_fetch_and_add(&num_of_threads, -1);
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
    //listenfd = 0;

    printf("got port!\n");
    //printf("num of countable chars is %d \n", NUM_OF_COUNTABLE_CHARS);
    for (int i=0; i < NUM_OF_COUNTABLE_CHARS; i++){
        counts.printable_chars_counts[i] = 0;
    }

    //initialize_pcc(counts);

    printf("initialize works!\n");

    // define vars for connection!
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset( &serv_addr, 0, addrsize );

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

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

    // TODO -- add signal handling function here

    while( continue_accept_sockets )
    {
        // accept
        int connfd = accept( socket_fd, NULL, NULL);

        // check if connfd works
        if( connfd < 0 )
        {
            // TODO -- need to check whether terminate by signal or by error
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            exit(-1);
        }

        printf("socket accepted!\n");

        //TODO -- export to another function
        // take care of client thread
        num_of_threads += 1;
        printf("num of threads is %d\n", num_of_threads);
        pthread_t new_thread;
        if (pthread_create(&new_thread, NULL, thread_action, (void*)connfd) != 0){
            printf("%s\n",strerror(errno));
            exit(-1);
        }

        while (num_of_threads > 0){
            // wait for all threads to finish
        }

        // print total_count
        for (int i=0; i<NUM_OF_COUNTABLE_CHARS; i++){
            printf("char '%c' : %u times \n", (char)(32 + i), counts.printable_chars_counts[i]);
        }

        // server finished well
        return 0;
    }
}
