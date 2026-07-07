#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define MAX_RETRIES 20

void nonblocking_read(const char* filename) {
    int fd = open(filename, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int retries = 0;
    
    printf("Non-Blocking I/O: Reading from %s\n", filename);
    printf("Will retry %d times with delays\n\n", MAX_RETRIES);
    
    while (retries < MAX_RETRIES) {
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("✓ Read %ld bytes after %d attempts: %s\n", 
                   bytes_read, retries + 1, buffer);
            break;
        } else if (bytes_read == 0) {
            printf("End of file reached\n");
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Attempt %d: No data available (EAGAIN/EWOULDBLOCK)\n", 
                       retries + 1);
                retries++;
                usleep(200000); // Sleep 200ms
            } else {
                perror("read");
                break;
            }
        }
    }
    
    if (retries >= MAX_RETRIES) {
        printf("Maximum retries reached, giving up\n");
    }
    
    close(fd);
}

int main(int argc, char* argv[]) {
    const char* filename = argc > 1 ? argv[1] : "test.txt";
    
    printf("=== Non-Blocking I/O Example ===\n");
    printf("Process ID: %d\n", getpid());
    printf("The read operation returns immediately with EAGAIN if no data\n\n");
    
    nonblocking_read(filename);
    
    return 0;
}