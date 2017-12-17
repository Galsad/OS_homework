#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
    if (argc != 3){
        printf("number of argument is not correct \n");
    }

    char* file_name = argv[1];
    int channel_id = atoi(argv[2]);

    int file_desc;
    int ret_val;

    char reading_buffer[128]  = {0};

    // TODO - need to verify it
    file_desc = open(file_name, O_RDWR);

    if (file_desc < 0){
        printf("can't open device file: %s", argv[1]);
        exit(-1);
    }

    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id);
    ret_val = read(file_desc, reading_buffer, MESSAGE_SIZE);
    ret_val = close(file_desc);

    if (ret_val == 0){
        printf("%s \n", reading_buffer);
        return 0;
    }

    printf("error occurred!\n");
    exit(-1);

}
