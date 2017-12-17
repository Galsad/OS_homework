#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
    if (argc != 4){
        printf("number of argument is not correct\n");
    }

    char* file_name = argv[1];
    int channel_id = atoi(argv[2]);
    char* message = argv[3];

    int file_desc;
    int ret_val;
    // TODO - need to verify it
    file_desc = open(file_name, O_RDWR);
    printf("file_desc is %d: \n", file_desc);

    if (file_desc < 0){
        printf("can't open device file: %s \n", argv[1]);
        exit(-1);
    }

    printf("before ioctl \n");
    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id);
    printf("before write \n");
    ret_val = write(file_desc, message, MESSAGE_SIZE);
    printf("before close \n");
    ret_val = close(file_desc);

    if (ret_val == 0){
        printf("writer successfully write a message\n");
        return 0;
    }

    printf("error occurred!\n");
    exit(-1);

}