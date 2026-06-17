#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT     8080
#define BUFSIZE  1024

int main() {
    int sockfd;
    char buffer[BUFSIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 1. Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   // accept on all interfaces
    server_addr.sin_port        = htons(PORT);

    // 3. Bind socket to port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP Server listening on port %d...\n", PORT);

    // 4. Echo loop
    while (1) {
        memset(buffer, 0, BUFSIZE);

        // Block until a datagram arrives; record sender's address
        int n = recvfrom(sockfd, buffer, BUFSIZE, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        printf("Received from %s:%d → \"%s\"\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               buffer);

        // Echo the message back
        if (sendto(sockfd, buffer, n, 0,
                   (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("sendto failed");
        }
    }

    close(sockfd);
    return 0;
}