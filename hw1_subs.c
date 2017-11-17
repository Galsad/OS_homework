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
    int closed = 0;

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

    int fd = open(file_path, O_RDONLY);
    if (fd < 0){
        printf("Problem with file reading...");
        closed = close(fd);
        return 1;
    }

    struct stat buffer;
    int file_status = fstat(fd, &buffer);

    int file_size = buffer.st_size;

    if (file_status < 0 ){
        printf("file status is not right");
        closed = close(fd);
        return 1;
    }

    char* str1 = argv[1];
    char* str2 = argv[2];

    char* reading_buffer = malloc((file_size+1)*sizeof(char));
    if (reading_buffer == NULL){
        free(reading_buffer);
        closed = close(fd);
        return 1;
    }

    char* tmp_buffer = malloc((file_size+1)*sizeof(char));
    if (tmp_buffer == NULL){
        free(tmp_buffer);
        free(reading_buffer);
        closed = close(fd);
        return 1;
    }

    // reading the file content into a buffer
    int file_pointer = 1;
    while (file_pointer != 0){
        file_pointer = read(fd, reading_buffer, file_size);
        if (file_pointer < 0) {
            free(tmp_buffer);
            free(reading_buffer);
            closed = close(fd);
            return 1;
        }
    }

    // replacing one string with another
    char* p = reading_buffer;
    int last_p = 0;

    while ((p=strstr(p, str1))){
        strncpy(tmp_buffer, reading_buffer + last_p, p-reading_buffer - last_p);
        
        tmp_buffer[p-reading_buffer - last_p] = '\0';
        printf("%s", tmp_buffer);
        printf("%s", str2);
        p ++;
        last_p = p - reading_buffer + strlen(str1) - 1;
    }

    strcpy(tmp_buffer, reading_buffer + last_p);
    printf("%s", tmp_buffer);

    free(reading_buffer);
    free(tmp_buffer);

    closed = close(fd);
    if (closed < 0){
        printf("file could not be closed!");
        return 1;
    }
    // program finished right
    return 0;
}


