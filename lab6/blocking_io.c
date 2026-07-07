#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1024

void blocking_read(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    printf("Blocking I/O: Reading from %s\n", filename);
    
    // This read will block until data is available
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Read %ld bytes: %s\n", bytes_read, buffer);
    } else if (bytes_read == 0) {
        printf("End of file reached\n");
    } else {
        perror("read");
    }
    
    close(fd);
}

int main(int argc, char* argv[]) {
    const char* filename = argc > 1 ? argv[1] : "test.txt";
    
    printf("=== Blocking I/O Example ===\n");
    printf("Process ID: %d\n", getpid());
    printf("The read operation will block until data is available\n\n");
    
    blocking_read(filename);
    
    return 0;
}