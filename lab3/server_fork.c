#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 2048

// Signal handler to prevent zombie processes
void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    printf("\n========================================\n");
    printf("Child process %d handling client\n", getpid());
    
    // Clear buffer
    memset(buffer, 0, BUFFER_SIZE);
    
    // Receive message from client
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Child %d received: %s\n", getpid(), buffer);
        
        // Calculate required size for response
        size_t response_size = strlen(buffer) + 100; // Extra space for header
        char *response = malloc(response_size);
        
        if (response != NULL) {
            // Now we can safely format without truncation
            snprintf(response, response_size, 
                     "Server (PID: %d): Received your message - %s", 
                     getpid(), buffer);
            
            printf("Child %d sending response\n", getpid());
            send(client_socket, response, strlen(response), 0);
            printf("Child %d sent response\n", getpid());
            
            free(response);
        } else {
            // Fallback if malloc fails
            char *error_msg = "Server: Memory allocation failed";
            send(client_socket, error_msg, strlen(error_msg), 0);
        }
    }
    
    sleep(2); // Simulate processing time
    close(client_socket);
    printf("Child %d finished\n", getpid());
    printf("========================================\n\n");
    exit(0);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Set up signal handler for zombie prevention
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(1);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("Concurrent server (PID: %d) listening on port %d...\n", getpid(), PORT);
    
    while (1) {
        // Accept new connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Fork a child process
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            close(client_socket);
            continue;
        }
        
        if (pid == 0) {
            // Child process
            close(server_socket); // Child doesn't need server socket
            handle_client(client_socket);
            exit(0);
        } else {
            // Parent process
            close(client_socket); // Parent doesn't need client socket
        }
    }
    
    close(server_socket);
    return 0;
}