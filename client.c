#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT atoi(argv[1])
#define SERVER_IP_ADDRESS argv[2]

#define BUFFER_SIZE 1024

int main(int argc, char const *argv[]) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    //
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }

    // Make a connection to the server with socket SendingSocket.
    int connectResult = connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        close(sock);
        return -1;
    }

    printf("Entering the chat.\n");
    printf("At any moment if you wish to leave , write exit.\n");
    printf("Enter message to send to server: \n");
    
    while (1) {
        // Get input message from user
        char input[BUFFER_SIZE];
        printf("Me: ");
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        // Remove newline character from input message
        input[strcspn(input, "\n")] = '\0';
        
        // Send message to server
        int messageLen = strlen(input) + 1;
        int bytesSent = send(sock, input, messageLen, 0);
        if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
            break;
        } else if (bytesSent == 0) {
            printf("peer has closed the TCP connection prior to send().\n");
            break;
        } else if (bytesSent < messageLen) {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        }
        else if(strcmp((input),"exit") == 0 || strcmp((input),"Exit") == 0){
                //Server choose to close chat
                break;
        }

        // printf("waiting for response from the server.\n");
        // Receive response from server
        char output[BUFFER_SIZE] = {'\0'};
        int bytesReceived = recv(sock, output, BUFFER_SIZE, 0);
        if (bytesReceived == -1) {
            printf("recv() failed with error code : %d", errno);
            break;
        } else if (bytesReceived == 0) {
            printf("the chat has been closed by server.\n");
            break;
        }
        else if(strcmp((output),"exit") == 0 || strcmp((output),"Exit") == 0){
                //Server choose to close chat
                break;
        }   
        else {
            printf("Server: %s", output);
        }
    }
    printf("Chat been closed.\n");
    close(sock);
    return 0;
}
