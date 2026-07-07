#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int main() {
    const char* filename = "test.txt";
    const char* content = "Hello, World! This is a test file for I/O models.\n"
                          "Line 2: Demonstrating various I/O operations.\n"
                          "Line 3: Blocking, non-blocking, multiplexing, and signal-driven I/O.\n";
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    ssize_t bytes_written = write(fd, content, strlen(content));
    if (bytes_written != (ssize_t)strlen(content)) {
        perror("write");
        close(fd);
        return 1;
    }
    
    close(fd);
    printf("Test file '%s' created successfully with %zu bytes\n", 
           filename, strlen(content));
    printf("Content:\n%s", content);
    
    return 0;
}