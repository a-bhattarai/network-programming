/*
 * signal_handling_server.c
 * Demonstrates signal handling in a network program:
 *   - SIGINT   : graceful server shutdown (Ctrl+C)
 *   - SIGCHLD  : reap terminated child processes (avoid zombies)
 *   - SIGPIPE  : ignore, so writing to a closed socket doesn't crash server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define BACKLOG 5
#define BUF_SIZE 1024

int server_fd; // global so signal handler can access it

// Handler for SIGINT: close socket and exit cleanly
void handle_sigint(int sig) {
    printf("\n[SIGINT] Caught signal %d. Shutting down server...\n", sig);
    close(server_fd);
    exit(0);
}

// Handler for SIGCHLD: reap all finished child processes
void handle_sigchld(int sig) {
    // WNOHANG: don't block if no child has exited yet
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        printf("[SIGCHLD] Reaped a terminated child process.\n");
    }
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 1. Register signal handlers
    signal(SIGINT, handle_sigint);     // Ctrl+C
    signal(SIGCHLD, handle_sigchld);   // child process termination
    signal(SIGPIPE, SIG_IGN);          // ignore broken pipe (writing to closed socket)

    // 2. Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        exit(1);
    }

    printf("Server listening on port %d (PID: %d)\n", PORT, getpid());
    printf("Press Ctrl+C to stop the server gracefully.\n");

    // 3. Accept loop — forks a child to handle each client
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            // accept() can be interrupted by a signal (e.g., SIGCHLD); just retry
            if (errno == EINTR) continue;
            perror("accept failed");
            continue;
        }

        printf("Connection accepted from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid == 0) {
            // Child process: handle client
            close(server_fd); // child doesn't need the listening socket

            char buffer[BUF_SIZE];
            int n = read(client_fd, buffer, BUF_SIZE - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Received: %s\n", buffer);
                write(client_fd, "Message received by server\n", 28);
            }

            close(client_fd);
            exit(0); // child exits -> triggers SIGCHLD in parent
        } else if (pid > 0) {
            // Parent process: close client_fd, loop back to accept()
            close(client_fd);
        } else {
            perror("fork failed");
        }
    }

    return 0;
}