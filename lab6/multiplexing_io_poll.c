#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_FDS 10

typedef struct {
    int fd;
    char name[32];
} file_info_t;

void multiplexing_read_poll(file_info_t files[], int num_files) {
    struct pollfd fds[MAX_FDS];
    char buffer[BUFFER_SIZE];
    
    // Initialize pollfd array
    for (int i = 0; i < num_files; i++) {
        fds[i].fd = files[i].fd;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }
    
    printf("Multiplexing I/O with poll: Monitoring %d files\n", num_files);
    printf("Timeout: 5 seconds\n\n");
    
    int ready = poll(fds, num_files, 5000); // 5 second timeout
    
    if (ready == -1) {
        perror("poll");
        return;
    } else if (ready == 0) {
        printf("Timeout occurred, no data available\n");
        return;
    }
    
    printf("Data available on %d file descriptor(s)\n", ready);
    
    // Check which file descriptors have data
    for (int i = 0; i < num_files; i++) {
        if (fds[i].revents & POLLIN) {
            ssize_t bytes_read = read(fds[i].fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("✓ Data from %s (fd=%d): %s\n", 
                       files[i].name, fds[i].fd, buffer);
            } else if (bytes_read == 0) {
                printf("End of file reached on %s\n", files[i].name);
            } else {
                perror("read");
            }
        }
    }
}

int main(int argc, char* argv[]) {
    file_info_t files[MAX_FDS];
    int num_files = 0;
    
    // Add stdin
    files[num_files].fd = STDIN_FILENO;
    strcpy(files[num_files].name, "stdin");
    num_files++;
    
    // Add files from command line
    for (int i = 1; i < argc && num_files < MAX_FDS; i++) {
        int fd = open(argv[i], O_RDONLY | O_NONBLOCK);
        if (fd != -1) {
            files[num_files].fd = fd;
            strcpy(files[num_files].name, argv[i]);
            num_files++;
        } else {
            perror("open");
        }
    }
    
    printf("=== Multiplexing I/O with poll Example ===\n");
    printf("Process ID: %d\n", getpid());
    printf("Monitoring: ");
    for (int i = 0; i < num_files; i++) {
        printf("%s ", files[i].name);
    }
    printf("\n\n");
    
    multiplexing_read_poll(files, num_files);
    
    // Cleanup
    for (int i = 1; i < num_files; i++) {
        close(files[i].fd);
    }
    
    return 0;
}