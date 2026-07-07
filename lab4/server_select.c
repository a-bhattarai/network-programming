#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30

int main() {
    int master_socket, new_socket, client_socket[MAX_CLIENTS];
    int activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    
    // Initialize all client sockets to 0
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }
    
    // Create master socket
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // Set up address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(master_socket, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Select-based server listening on port %d\n", PORT);
    printf("Waiting for connections...\n");
    
    socklen_t addrlen = sizeof(address);
    
    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);
        
        // Add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
        
        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            
            // If valid socket descriptor, add to read list
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            
            // Highest file descriptor number, need it for select()
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
        
        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        
        if ((activity < 0) && (errno != EINTR)) {
            printf("select error\n");
        }
        
        // If something happened on the master socket,
        // it's an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            new_socket = accept(master_socket, (struct sockaddr *)&address, &addrlen);
            if (new_socket < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            
            printf("New connection from %s:%d, socket fd: %d\n",
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port), new_socket);
            
            // Add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }
        
        // Otherwise, it's some I/O operation on a client socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            
            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing (client disconnected)
                valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);
                
                if (valread == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&address, &addrlen);
                    printf("Client %s:%d disconnected\n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    
                    // Close the socket and mark as 0 in list
                    close(sd);
                    client_socket[i] = 0;
                } else if (valread > 0) {
                    // Process the message
                    buffer[valread] = '\0';
                    
                    // Get client address
                    getpeername(sd, (struct sockaddr*)&address, &addrlen);
                    printf("Received from %s:%d: %s\n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port), buffer);
                    
                    // Echo the message back
                    char response[BUFFER_SIZE];
                    int response_len = snprintf(response, BUFFER_SIZE,
                            "Server echo: %s\n", buffer);
                    
                    // Ensure we only send what was actually written
                    if (response_len >= BUFFER_SIZE) {
                        // Truncate if too long
                        response[BUFFER_SIZE - 1] = '\0';
                        response_len = BUFFER_SIZE - 1;
                    }
                    
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }
    
    close(master_socket);
    return 0;
}