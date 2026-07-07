#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

volatile sig_atomic_t io_ready = 0;
char buffer[BUFFER_SIZE];
int data_fd = -1;

void io_handler(int sig) {
    if (sig == SIGIO) {
        io_ready = 1;
        printf("\nSignal received: I/O is ready!\n");
        
        // Read data in the signal handler (not recommended for complex operations)
        ssize_t bytes_read = read(data_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Data read in signal handler: %s\n", buffer);
        } else if (bytes_read == 0) {
            printf("End of file reached\n");
        } else {
            perror("read in signal handler");
        }
    }
}

void setup_signal_driven_io(const char* filename) {
    struct sigaction sa;
    
    // Open file
    data_fd = open(filename, O_RDONLY | O_NONBLOCK);
    if (data_fd == -1) {
        perror("open");
        return;
    }
    
    // Set up signal handler for SIGIO
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = io_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGIO, &sa, NULL);
    
    // Set the process ID to receive SIGIO
    if (fcntl(data_fd, F_SETOWN, getpid()) == -1) {
        perror("fcntl F_SETOWN");
        close(data_fd);
        return;
    }
    
    // Enable asynchronous notification
    int flags = fcntl(data_fd, F_GETFL);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        close(data_fd);
        return;
    }
    
    if (fcntl(data_fd, F_SETFL, flags | FASYNC) == -1) {
        perror("fcntl F_SETFL");
        close(data_fd);
        return;
    }
    
    printf("Signal-driven I/O setup complete for %s\n", filename);
    printf("Waiting for SIGIO signals...\n");
    printf("(Data read will happen in the signal handler)\n\n");
}

int main(int argc, char* argv[]) {
    const char* filename = argc > 1 ? argv[1] : "test.txt";
    
    printf("=== Signal-Driven I/O Example ===\n");
    printf("Process ID: %d\n", getpid());
    printf("This process will receive SIGIO when data is ready\n\n");
    
    setup_signal_driven_io(filename);
    
    // Main loop
    int count = 0;
    while (count < 30) { // Run for 30 seconds max
        sleep(1);
        count++;
        
        if (count % 5 == 0) {
            printf("Main process still running... (%d seconds)\n", count);
        }
        
        // Note: In a real application, you'd process the data here
        // But we're reading it in the signal handler for simplicity
    }
    
    printf("\nMain process exiting\n");
    
    if (data_fd != -1) {
        close(data_fd);
    }
    
    return 0;
}