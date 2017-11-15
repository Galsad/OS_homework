#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

    if (argc != 3){
        printf("3 arguments required!");
        return 1;
    }

    char* hw1_dir = "HW1DIR";


    // check if hw1 is defined
    if (!getenv(hw1_dir)){
        printf("HW1DIR is not defined\n");
        return 1;
    }

    // check if hw1_tf is defined
    char* hw1_tf = "HW1TF";

    if (!getenv(hw1_tf)) {
        printf("HW1TF is not defined\n");
        return 1;
    }

    // generate file path
    char file_path[2048];
    strcpy(file_path, getenv(hw1_dir));
    strcat(file_path, "/");
    strcat(file_path, getenv(hw1_tf));

    // try to open file path
    struct stat buffer;
    int fd = open(file_path, O_RDONLY);
    if (fd < 0){
        printf("Problem with file reading...");
        return 1;
    }

    int file_status = fstat(fd, &buffer);

    if (file_status < 0 ){
        printf("file status is not right");
        return 1;
    }

    int file_size = buffer.st_size;
    printf("file size is %d\n", file_size);

    char* str1 = argv[1];
    char* str2 = argv[2];

    char* reading_buffer = malloc((file_size+1)*sizeof(char));
    char* tmp_buffer = malloc((file_size+1)*sizeof(char));

    char* reading_buffer = malloc((file_size+1)*sizeof(char));
    char* tmp_buffer = malloc((file_size+1)*sizeof(char));

    // reading the file content into a buffer
    int file_pointer = 0;
    while (file_pointer  = read(fd, reading_buffer, file_size)){
        reading_buffer[file_pointer ] = '\0';
    }

    printf("new string is: %s\n", reading_buffer);
    printf("str1 is %s\nstr2 is %s\n", str1, str2);

    // replacing one string with another
    char* p = reading_buffer;

    while ((p=strstr(p, str1))){
        // read from the next str1 - copy to buf the orig_str till str1
        strncpy(tmp_buffer, reading_buffer, p-reading_buffer);
        tmp_buffer[p-reading_buffer] = '\0';
        // add to buffer str2
        strcat(tmp_buffer, str2);
        strcat(tmp_buffer, p + strlen(str1));
        strcpy(reading_buffer, tmp_buffer);
        p++;
    }

    printf("new string is: %s", tmp_buffer);
    free(tmp_buffer);
    free(reading_buffer);

    int closed = close(fd);
    if (suc < 0){
        printf("file could not be closed!");
        return 1;
    }
    // program finished right
    return 0;
}


