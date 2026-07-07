#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    
    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    printf("Connected to server. Enter messages (type 'quit' to exit):\n");
    
    while (1) {
        printf("> ");
        
        // Get user input with size checking
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Remove newline and check for empty input
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        // If input is empty, continue
        if (strlen(buffer) == 0) {
            continue;
        }
        
        // Check for quit command
        if (strcmp(buffer, "quit") == 0) {
            printf("Exiting...\n");
            break;
        }
        
        // Send message to server
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            break;
        }
        
        // Receive response with size checking
        int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';  // Safe: bytes <= BUFFER_SIZE - 1
            printf("Server: %s\n", buffer);
        } else if (bytes == 0) {
            printf("Server disconnected\n");
            break;
        } else {
            perror("recv");
            break;
        }
    }
    
    close(sock);
    return 0;
}