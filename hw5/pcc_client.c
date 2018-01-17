//
// Created by gal on 1/12/18.
//
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <sys/mman.h>


// checks if the ip is valid one or not
int valid_digit(char *ip_str)
{
    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9')
            ++ip_str;
        else
            return 0;
    }
    return 1;
}

/* return true if IP string is valid, else return false */
bool is_valid_ip(char *ip_str) {
    int num, dots = 0;
    char *ptr;

    if (ip_str == NULL)
        return false;

    ptr = strtok(ip_str, ".");

    if (ptr == NULL)
        return false;

    while (ptr) {

        /* after parsing string, it must contain only digits */
        if (!valid_digit(ptr))
            return false;

        num = atoi(ptr);

        /* check for valid IP */
        if (num >= 0 && num <= 255) {
            /* parse remaining string */
            ptr = strtok(NULL, ".");
            if (ptr != NULL)
                ++dots;
        } else
            return false;
    }

    /* valid IP string must contain 3 dots */
    if (dots != 3)
        return false;
    return true;
}


void read_from_devurandom(int n, char* random_data_buffer){
    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData < 0)
    {
        printf("ERROR: %s\n", strerror(errno));
        exit(-1);
    }

    else
    {
        ssize_t result = read(randomData, random_data_buffer, n);
        if (result < 0)
        {
            printf("ERROR: %s\n", strerror(errno));
            exit(-1);
        }
    }
}

int main(int argc, char* argv[]){

    if (argc != 4){
        printf("usage - server name / ip, host, number of bytes to send\n");
        exit(-1);
    }

    struct addrinfo hints, *infoptr;
    memset(&hints, 0, sizeof(struct addrinfo));

    // vars init
    int  sockfd     = -1;
    int  bytes_read =  0;
    int count_from_server;

    struct sockaddr_in serv_addr; // where we Want to get to

    // ip add is defined twice for checking if legal IP
    char* ip_add = argv[1];
    char ip_add2[32];
    strcpy(ip_add2, ip_add);

    int port = atoi(argv[2]);
    int num_of_bytes_to_read = atoi(argv[3]);

    char* random_bytes_buffer = malloc(sizeof(char)*num_of_bytes_to_read);
    int nsent = 0;
    int success = 0;

    // got IP
    if (is_valid_ip(ip_add) == true){
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        serv_addr.sin_addr.s_addr = inet_addr(ip_add2);

        if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Error : %s \n", strerror(errno));
            return 1;
        }

        if( connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\n Error : Connect Failed. %s \n", strerror(errno));
            return 1;
        }
    }

    // got host name and not IP address
    else{
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;

        success = getaddrinfo(argv[1], argv[2], &hints, &infoptr);
        if (success){
            printf("\n Error : Could not connect %s\n", strerror(errno));
            exit(-1);
        }
        // creates a buffer for server response!

        if( (sockfd = socket(infoptr->ai_family, infoptr->ai_socktype, infoptr->ai_protocol)) < 0)
        {
            printf("\n Error : Could not create socket %s\n", strerror(errno));
            exit(-1);
        }

        // connect
        if( connect(sockfd, (struct sockaddr*) (infoptr->ai_addr), infoptr->ai_addrlen) < 0)
        {
            printf("\n Error : Connect Failed. %s \n", strerror(errno));
            exit(-1);
        }
    }

    read_from_devurandom(num_of_bytes_to_read ,random_bytes_buffer);

    // send random data to server
    int totalsent = 0;
    int notwritten = num_of_bytes_to_read;

    // first send the length of the stream
    while( notwritten > 0 )
    {
        nsent = send(sockfd, random_bytes_buffer + totalsent, num_of_bytes_to_read, 0);

        totalsent  += nsent;
        notwritten -= nsent;
    }

    //enable the socket listen again
    shutdown(sockfd, SHUT_WR);

    bytes_read = read(sockfd, &count_from_server, sizeof(unsigned int));

    //read from server...
    int real_amount_of_bytes = bytes_read;
    while ( bytes_read < sizeof(unsigned int)){
        bytes_read = read(sockfd, ((char*) &count_from_server) + real_amount_of_bytes, sizeof(unsigned int) - real_amount_of_bytes);
        real_amount_of_bytes += bytes_read;
    }

    printf("# of printable characters: %u\n", count_from_server);

    close(sockfd); // is socket really done here?
    return 0;
}