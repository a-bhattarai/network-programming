#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server. Type your commands:\n");
    
    // Communication loop
    while (1) {
        // Receive server messages
        ssize_t bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            printf("Server disconnected\n");
            break;
        }
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
        
        // Get user input
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            // Send to server
            send(sock, buffer, strlen(buffer), 0);
        }
    }
    
    close(sock);
    return 0;
}