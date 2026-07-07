#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Function prototypes
void* handle_client(void* arg);
void sigint_handler(int sig);

// Global flag for server shutdown
volatile sig_atomic_t server_running = 1;

// Safe send function
int safe_send(int socket, const char* message) {
    if (message == NULL) return -1;
    return send(socket, message, strlen(message), 0);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pthread_t thread_id;
    
    // Set up signal handler
    signal(SIGINT, sigint_handler);
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", PORT);
    printf("Press Ctrl+C to stop the server\n");
    
    // Accept clients in a loop
    while (server_running) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (server_running) {
                perror("Accept failed");
            }
            continue;
        }
        
        // Get client IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("New connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        
        // Create a thread to handle the client
        int* client_socket_ptr = malloc(sizeof(int));
        if (client_socket_ptr == NULL) {
            perror("Memory allocation failed");
            close(client_socket);
            continue;
        }
        *client_socket_ptr = client_socket;
        
        if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) < 0) {
            perror("Thread creation failed");
            close(client_socket);
            free(client_socket_ptr);
            continue;
        }
        
        // Detach thread so it cleans up automatically
        pthread_detach(thread_id);
    }
    
    // Clean up
    close(server_socket);
    printf("Server shutdown complete.\n");
    return 0;
}

// Signal handler for graceful shutdown
void sigint_handler(int sig) {
    printf("\nShutting down server...\n");
    server_running = 0;
}

// Function to handle client communication
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg); // Free the allocated memory
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // Send welcome message
    const char* welcome_msg = "Welcome to the Multi-threaded Server!\n"
                             "Type 'quit' to disconnect\n"
                             "Type 'echo' to echo back your message\n"
                             "Type 'time' to get server time\n"
                             "Type 'help' for available commands\n"
                             "> ";
    safe_send(client_socket, welcome_msg);
    
    // Handle client messages
    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        // Remove newline character if present
        buffer[strcspn(buffer, "\n")] = '\0';
        buffer[strcspn(buffer, "\r")] = '\0';
        
        // Process commands
        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
            printf("Client disconnected\n");
            safe_send(client_socket, "Goodbye!\n");
            break;
        } 
        else if (strcmp(buffer, "echo") == 0) {
            safe_send(client_socket, "Enter message to echo: ");
            
            // Receive the message to echo
            ssize_t echo_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            if (echo_bytes > 0) {
                buffer[echo_bytes] = '\0';
                buffer[strcspn(buffer, "\n")] = '\0';
                
                char response[BUFFER_SIZE + 50];
                snprintf(response, sizeof(response), "Echo: %s\n> ", buffer);
                safe_send(client_socket, response);
            }
        }
        else if (strcmp(buffer, "time") == 0) {
            time_t now;
            time(&now);
            char* time_str = ctime(&now);
            
            char response[BUFFER_SIZE + 50];
            snprintf(response, sizeof(response), "Server time: %s> ", time_str);
            safe_send(client_socket, response);
        }
        else if (strcmp(buffer, "help") == 0) {
            const char* help_msg = "Available commands:\n"
                                   "  quit    - Disconnect from server\n"
                                   "  echo    - Echo back your message\n"
                                   "  time    - Get server time\n"
                                   "  help    - Show this help message\n"
                                   "> ";
            safe_send(client_socket, help_msg);
        }
        else {
            char response[BUFFER_SIZE + 100];
            snprintf(response, sizeof(response), "Unknown command: '%s'. Type 'help' for available commands.\n> ", buffer);
            safe_send(client_socket, response);
        }
    }
    
    close(client_socket);
    printf("Client connection closed\n");
    return NULL;
}