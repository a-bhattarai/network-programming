#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
#define MAX_FDS 10

typedef struct {
    int fd;
    char name[32];
} file_info_t;

void multiplexing_read_select(file_info_t files[], int num_files) {
    fd_set readfds;
    int max_fd = 0;
    struct timeval timeout;
    char buffer[BUFFER_SIZE];
    
    // Initialize fd_set
    FD_ZERO(&readfds);
    
    // Add all file descriptors to the set
    for (int i = 0; i < num_files; i++) {
        FD_SET(files[i].fd, &readfds);
        if (files[i].fd > max_fd) {
            max_fd = files[i].fd;
        }
    }
    
    // Set timeout to 5 seconds
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    printf("Multiplexing I/O with select: Monitoring %d files\n", num_files);
    printf("Timeout: 5 seconds\n\n");
    
    int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
    
    if (ready == -1) {
        perror("select");
        return;
    } else if (ready == 0) {
        printf("Timeout occurred, no data available\n");
        return;
    }
    
    printf("Data available on %d file descriptor(s)\n", ready);
    
    // Check which file descriptors have data
    for (int i = 0; i < num_files; i++) {
        if (FD_ISSET(files[i].fd, &readfds)) {
            ssize_t bytes_read = read(files[i].fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("✓ Data from %s (fd=%d): %s\n", 
                       files[i].name, files[i].fd, buffer);
            } else if (bytes_read == 0) {
                printf("End of file reached on %s\n", files[i].name);
            } else {
                perror("read");
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Example: monitor stdin and a file
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
    
    printf("=== Multiplexing I/O with select Example ===\n");
    printf("Process ID: %d\n", getpid());
    printf("Monitoring: ");
    for (int i = 0; i < num_files; i++) {
        printf("%s ", files[i].name);
    }
    printf("\n\n");
    
    multiplexing_read_select(files, num_files);
    
    // Cleanup
    for (int i = 1; i < num_files; i++) {
        close(files[i].fd);
    }
    
    return 0;
}