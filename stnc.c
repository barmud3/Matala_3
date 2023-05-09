#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/wait.h>

void start_client(char* ip, int port) {
    int pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        return;
    } else if (pid == 0) {

        // Child process
        char port_str[10];
        sprintf(port_str, "%d", port);
        char* args[] = {"./client", port_str, ip, NULL};
        execv("./client", args);
        // This line will only be executed if execv fails to start the program
        perror("execv() failed");
        exit(1);

    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

void start_server(int port) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork() failed");
        exit(1);
    } else if (pid == 0) { // Child process
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%d", port);

        char *args[] = {"./server", port_str, NULL};
        execv(args[0], args);

        // If execv returns, there was an error
        perror("execv() failed");
        exit(1);
    } else { // Parent process
        // Wait for the child process to terminate
        int status;
        waitpid(pid, &status, 0);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s [-s PORT] | [-c IP PORT]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int opt;
    char* ip = NULL;
    int port = 0;

    while ((opt = getopt(argc, argv, "c:s:")) != -1) {
        switch (opt) {
            //client
            case 'c':
                ip = argv[2];
                port = atoi(argv[3]);
                start_client(ip, port);
                break;
            case 's':
                port = atoi(argv[2]);
                start_server(port);
                break;
            default:
                printf("Usage: %s [-c IP PORT] | [-s PORT]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    return 0;
}
