#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/stat.h>

void start_client(char* ip, int port) {
    int pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        return;
    } else if (pid == 0) {

        // Child process
        char port_str[10];
        sprintf(port_str, "%d", port);
        char* args[] = {"./client", port_str, ip,NULL};
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
void start_client_performance(char* ip, int port , char* cType , char* pType) {

    const char* pipeName = pType;

    if(strcmp("pipe",cType) == 0){
        // Create the named pipe
        const char* pipeName = pType;
        if (mkfifo(pipeName, 0666) == -1) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    int pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        return;
    } else if (pid == 0) {

        // Child process
        char port_str[10];
        sprintf(port_str, "%d", port);
        char* args[] = {"./client", port_str, ip, cType, pType, NULL};
        execv("./client", args);
        // This line will only be executed if execv fails to start the program
        perror("execv() failed");
        exit(1);

    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }

    if(strcmp("pipe",cType) == 0){
        // Remove the named pipe
        if (unlink(pipeName) == -1) {
            perror("unlink");
            exit(EXIT_FAILURE);
        }
    }
}
void start_server(int port , int performance) {

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork() failed");
        exit(1);
    } else if (pid == 0) { // Child process
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%d", port);

        char performance_str[16];
        snprintf(performance_str, sizeof(performance_str), "%d", performance);

        char *args[] = {"./server", port_str , performance_str , NULL};
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
    
    if (argc < 3 || argc > 7 || (strcmp(argv[1], "-c") != 0 && strcmp(argv[1], "-s") != 0)){
        printf("Client usage: %s [-c] [IP PORT] [-p PARAMETER TYPE]\n", argv[0]);
        printf("Server usage: %s [-s] [PORT] [-p] [-q]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // else if (argc < 3 || argc > 7 || strcmp(argv[1], "-s") != 0){
    //     printf("Client usage: %s [-c] [IP PORT] [-p PARAMETER TYPE]\n", argv[0]);
    //     printf("Server usage: %s [-s] [PORT] [-p] [-q]\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }


    int flag = 1;
    if(strcmp(argv[1], "-c") == 0){
        if(argc > 6){
            if(strcmp(argv[4], "ipv4") == 0 && strcmp(argv[5], "tcp") !=0 || strcmp(argv[5], "udp") != 0){
                flag = 0;
            }
            else if(strcmp(argv[4], "ipv6") == 0 && strcmp(argv[5], "tcp") !=0 || strcmp(argv[5], "udp") != 0){
                flag = 0;
            }
            else if(strcmp(argv[4], "uds") == 0 && strcmp(argv[5], "dgram") !=0 || strcmp(argv[5], "stream") != 0){
                flag = 0;
            }
            if(strcmp(argv[4], "mmap") == 0 && strcmp(argv[4], "pipe") == 0){
                flag = 1;
            }
        }
    }  
    if(strcmp(argv[1], "-s") == 0){
        if(argc > 3){
            if(strcmp(argv[3], "-p") != 0){
                flag = 0;
            }
            else if(argc > 4){
                if(strcmp(argv[4], "-q") !=0 ){
                    flag = 0;
                }
            }
        }
    }

    if(flag == 0){
            printf("Client usage: %s [-c] [IP PORT] [-p PARAMETER TYPE]\n", argv[0]);
            printf("Server usage: %s [-s] [PORT] [-p] [-q]\n", argv[0]);
            exit(EXIT_FAILURE);
    }  
 
    int opt;
    char* ip = NULL;
    int port = 0;
    int flagc = 0;
    int flags = 0;
    int flagp = 0;
    int flagq = 0;
    for (int i = 1; i < argc; i++) {
            
            if(strcmp(argv[i], "-c") == 0)
            {
                flagc = 1;
            }
            else if(strcmp(argv[i], "-s") == 0)
            {
                flags = 1;
            }
            else if(strcmp(argv[i], "-p") == 0)
            {
                flagp = 1;
            }
            else if(strcmp(argv[i], "-q") == 0)
            {
                flagq = 1;
            }
        }
        if (flagc == 1)
        {
            ip = argv[2];
            port = atoi(argv[3]);
           if (flagp == 1)
           {
                char* cType = argv[5];
                char* pType = argv[6];
                start_client_performance(ip, port, cType, pType);
           }
           else{
            start_client(ip, port);
           }
        }
        else if (flags == 1)
        {
            port = atoi(argv[2]);
            if (flagp == 1)
            {
                if(flagq == 1){
                    start_server(port,2);   
                }
                    start_server(port,1);
            }
            else{
                start_server(port,0);
            }
    }
    return 0;
}
