/*
 * daemon_process.c
 * Demonstrates how to daemonize a process in C (classic UNIX double-fork method)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

#define LOG_FILE "/tmp/mydaemon.log"

void daemonize() {
    pid_t pid;

    // Step 1: First fork — parent exits, child continues
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent process exits
        exit(EXIT_SUCCESS);
    }

    // Step 2: Child becomes session leader (detaches from controlling terminal)
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Step 3: Ignore signals that could stop the daemon unexpectedly
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    // Step 4: Second fork — ensures daemon can never re-acquire a terminal
    pid = fork();
    if (pid < 0) {
        perror("second fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS); // first child exits
    }

    // Step 5: Set new file permissions mask
    umask(0);

    // Step 6: Change working directory to root (avoid locking any directory)
    chdir("/");

    // Step 7: Close standard file descriptors (stdin, stdout, stderr)
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Step 8: Redirect stdin/stdout/stderr to /dev/null (or a log file)
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_RDWR);   // stderr
}

int main() {
    daemonize();

    // From here on, this process runs in the background as a daemon
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
        exit(EXIT_FAILURE);
    }

    // Main daemon loop — just logs a timestamp every 10 seconds
    while (1) {
        time_t now = time(NULL);
        fprintf(log, "Daemon running at: %s", ctime(&now));
        fflush(log);
        sleep(10);
    }

    fclose(log);
    return 0;
}