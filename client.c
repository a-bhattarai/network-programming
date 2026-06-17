#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP  "127.0.0.1"
#define PORT       8080
#define BUFSIZE    1024

int main() {
    int sockfd;
    char send_buf[BUFSIZE], recv_buf[BUFSIZE];
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);

    // 1. Create UDP socket (no connection established)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Configure server address to send to
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3. Send/receive loop
    while (1) {
        printf("Enter message (or 'quit' to exit): ");
        if (fgets(send_buf, BUFSIZE, stdin) == NULL) break;

        // Strip trailing newline
        send_buf[strcspn(send_buf, "\n")] = '\0';

        if (strcmp(send_buf, "quit") == 0) break;

        // 4. Send datagram — no prior connect() needed
        if (sendto(sockfd, send_buf, strlen(send_buf), 0,
                   (struct sockaddr *)&server_addr, server_len) < 0) {
            perror("sendto failed");
            continue;
        }

        // 5. Wait for echo reply
        memset(recv_buf, 0, BUFSIZE);
        int n = recvfrom(sockfd, recv_buf, BUFSIZE, 0,
                         (struct sockaddr *)&server_addr, &server_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        printf("Echo from server: \"%s\"\n", recv_buf);
    }

    close(sockfd);
    printf("Client exiting.\n");
    return 0;
}