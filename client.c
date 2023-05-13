#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>


#define SERVER_PORT atoi(argv[1])
#define SERVER_IP_ADDRESS argv[2]
#define COMMUNICATION_TYPE argv[3]
#define PARAMETER_TYPE argv[4]
#define BUFFER_SIZE 6400
#define MAX_PACKET_SIZE_UDP 64000
#define MAX_PACKET_SIZE_UDS_DGRAM 212500
#define CHUNK_SIZE (100 * 1024 * 1024)
#define SOCKET_PATH "/tmp/my_unix_socket"

unsigned int calculate_checksum(char* buffer, size_t size) {
    unsigned int sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum += (unsigned int)buffer[i];
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

unsigned char* generate_data() {
    unsigned char* data = malloc(CHUNK_SIZE);
    if (data == NULL) {
        printf("Failed to allocate memory for data.\n");
        exit(1);
    }
    memset(data, 'a', CHUNK_SIZE);
    return data;
}

void handleIPv4TCPClient(int port) {

    int clientSocket;
    struct sockaddr_in serverAddr;

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Client socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); 
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connection established with the server!\n");

    // Generate data
    const size_t dataSize = CHUNK_SIZE;
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    generate_data(dataBuffer, dataSize);

    // Calculate checksum
    unsigned int checksum = calculate_checksum(dataBuffer, dataSize);

    // Send data to the client
    size_t sentBytes = 0;
    while (sentBytes < dataSize) {
        ssize_t bytes = send(clientSocket, dataBuffer + sentBytes, BUFFER_SIZE, 0);
        if (bytes < 0) {
            perror("Sending failed");
            exit(EXIT_FAILURE);
        }
        sentBytes += bytes;
    }

    // Send checksum to the client
    unsigned int networkChecksum = htonl(checksum);
    send(clientSocket, &networkChecksum, sizeof(unsigned int), 0);

    printf("Data sent successfully! Total bytes sent: %zu\n", sentBytes);

    // Close the socket when finished
    close(clientSocket);
    free(dataBuffer);
}

void handleIPv6TCPClient(int port) {

    int clientSocket;
    struct sockaddr_in6 serverAddr;

    // Create socket
    clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Client socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(port);  
    inet_pton(AF_INET6, "::1", &(serverAddr.sin6_addr));

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connection established with the server!\n");

    // Generate data
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    generate_data(dataBuffer, CHUNK_SIZE);

    // Calculate checksum
    unsigned int checksum = calculate_checksum(dataBuffer, CHUNK_SIZE);

    ssize_t bytesSent = send(clientSocket, dataBuffer, CHUNK_SIZE, 0);
    if (bytesSent < 0) {
        perror("Sending data failed");
        exit(EXIT_FAILURE);
    }

    printf("Data sent successfully! Total bytes sent: %zu\n", bytesSent);

    // Close the socket when finished
    close(clientSocket);
    free(dataBuffer);
}

void handleIPv4UDPClient(int port , const char* serverIP) {
    // Handle ipv4 tcp client communication
    int clientSocket;
    struct sockaddr_in serverAddr;

    // Create socket
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket < 0) {
        perror("Client socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); 
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr));

    printf("Connection established with the server!\n");

    // Generate data
    size_t dataSize = CHUNK_SIZE;
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    generate_data(dataBuffer, CHUNK_SIZE);

    // Calculate checksum
    uint16_t checksum = calculate_checksum(dataBuffer, CHUNK_SIZE);

    // Calculate the number of parts to send
    size_t totalBytesSent = 0;
    size_t partSize;
    char buffer[MAX_PACKET_SIZE_UDP];
    while (1) {
        partSize = (dataSize - totalBytesSent > MAX_PACKET_SIZE_UDP) ? MAX_PACKET_SIZE_UDP : (dataSize - totalBytesSent);
        // Create a buffer for the current part
        memcpy(buffer, dataBuffer + totalBytesSent, partSize);
        // Send the current part to the server
        ssize_t bytesSentPart = sendto(clientSocket, buffer, partSize, 0,
                                       (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (bytesSentPart < 0) {
            perror("Sending data failed");
            exit(EXIT_FAILURE);
        }
        if (bytesSentPart == 0) {
            printf("Data sent successfully! Total bytes sent: %zu\n", bytesSentPart);
            exit(EXIT_FAILURE);
        }
        totalBytesSent+=bytesSentPart;
        memset(&buffer, 0, sizeof(buffer));
        partSize = (dataSize - totalBytesSent > MAX_PACKET_SIZE_UDP) ? MAX_PACKET_SIZE_UDP : (dataSize - totalBytesSent);
        
    }

    // Close the socket when finished
    close(clientSocket);
    free(dataBuffer);
}

void handleIPv6UDPClient(int port , const char* serverIP) {
    
    int clientSocket;
    struct sockaddr_in6 serverAddr;

    // Create socket
    clientSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket < 0) {
        perror("Client socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(port); 
    inet_pton(AF_INET6, serverIP, &(serverAddr.sin6_addr));

    printf("Connection established with the server!\n");

    // Generate data
    size_t dataSize = CHUNK_SIZE;
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    generate_data(dataBuffer, CHUNK_SIZE);

    // Calculate checksum
    uint16_t checksum = calculate_checksum(dataBuffer, CHUNK_SIZE);

    // Calculate the number of parts to send
    size_t totalBytesSent = 0;
    size_t partSize;
    char buffer[MAX_PACKET_SIZE_UDP];
    while (1) {
        partSize = (dataSize - totalBytesSent > MAX_PACKET_SIZE_UDP) ? MAX_PACKET_SIZE_UDP : (dataSize - totalBytesSent);
        // Create a buffer for the current part
        memcpy(buffer, dataBuffer + totalBytesSent, partSize);
        // Send the current part to the server
        ssize_t bytesSentPart = sendto(clientSocket, buffer, partSize, 0,
                                       (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (bytesSentPart < 0) {
            perror("Sending data failed");
            exit(EXIT_FAILURE);
        }
        if (bytesSentPart == 0) {
            printf("Data sent successfully! Total bytes sent: %zu\n", bytesSentPart);
            exit(EXIT_FAILURE);
        }
        totalBytesSent+=bytesSentPart;
        memset(&buffer, 0, sizeof(buffer));
        partSize = (dataSize - totalBytesSent > MAX_PACKET_SIZE_UDP) ? MAX_PACKET_SIZE_UDP : (dataSize - totalBytesSent);
        
    }

    // Close the socket when finished
    close(clientSocket);
    free(dataBuffer);
}

void handleUnixStreamClient() {

    // Create socket
    int clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Client socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    struct sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, SOCKET_PATH);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    printf("Connection established with the server!\n");

    // Generate data
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    generate_data(dataBuffer, CHUNK_SIZE);
    

    // Send data to the server
    ssize_t bytesSent = send(clientSocket, dataBuffer, CHUNK_SIZE, 0);
    if (bytesSent < 0) {
        perror("Sending data failed");
        exit(EXIT_FAILURE);
    }


    printf("Data sent successfully! Total bytes sent: %ld\n", bytesSent);

    // Close the socket when finished
    close(clientSocket);
    free(dataBuffer);
}

void handleUnixDgramClient() {

    // Create socket
    int clientSocket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        perror("Client socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    struct sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, SOCKET_PATH);

    // Generate data
    size_t dataSize = CHUNK_SIZE;
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    generate_data(dataBuffer, CHUNK_SIZE);

    size_t totalBytesSent = 0;
    size_t partSize = (dataSize - totalBytesSent > MAX_PACKET_SIZE_UDS_DGRAM) ? MAX_PACKET_SIZE_UDS_DGRAM : (dataSize - totalBytesSent);
    socklen_t serverAddrLen = sizeof(serverAddr);
    while (partSize > 0) {
        
        // Create a buffer for the current part
        char buffer[MAX_PACKET_SIZE_UDS_DGRAM];
        memcpy(buffer, dataBuffer + totalBytesSent, partSize);
        // Send the current part to the server
        ssize_t bytesSentPart = sendto(clientSocket, buffer, partSize, 0,
                                       (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (bytesSentPart < 0) {
            perror("Sending data failed");
            exit(EXIT_FAILURE);
        }
        totalBytesSent+=bytesSentPart;

        partSize = (dataSize - totalBytesSent > MAX_PACKET_SIZE_UDS_DGRAM) ? MAX_PACKET_SIZE_UDS_DGRAM : (dataSize - totalBytesSent);
    }

    printf("Data sent successfully! Total bytes sent: %zu\n", totalBytesSent);

    // Close the socket when finished
    close(clientSocket);
    free(dataBuffer);
}

void handleMmapClient(char* filename) {

    size_t dataSize = CHUNK_SIZE;

    // Create a file to be memory-mapped
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Extend the file size to the desired size
    if (lseek(fd, CHUNK_SIZE - 1, SEEK_SET) == -1) {
        close(fd);
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    if (write(fd, "", 1) == -1) {
        close(fd);
        perror("write");
        exit(EXIT_FAILURE);
    }

    // Memory map the file
    char* data = mmap(NULL, CHUNK_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    memcpy(data, generate_data(), CHUNK_SIZE);

    printf("Data written successfully. Total bytes written: %zu\n", dataSize);

    close(fd);
}

void handleNamedPipeClient(char* pipeName) {

    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytesWritten;
    size_t totalBytesWritten = 0;

    // Open named pipe for writing
    fd = open(pipeName, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Generate data
    size_t dataSize = CHUNK_SIZE;
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    generate_data(dataBuffer, CHUNK_SIZE);

    // Write the generated data into the named pipe
    while (totalBytesWritten < CHUNK_SIZE) {
        
        size_t remainingBytes = CHUNK_SIZE - totalBytesWritten;
        size_t bytesToWrite = (remainingBytes < BUFFER_SIZE) ? remainingBytes : BUFFER_SIZE;
        bytesWritten = write(fd, dataBuffer + totalBytesWritten, bytesToWrite);
        if (bytesWritten == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        totalBytesWritten += bytesWritten;
    }
    

    printf("Data written successfully. Total bytes written: %zu\n", dataSize);

    // Clean up and close the named pipe
    close(fd);
}

int main(int argc, char const* argv[]) {

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    int rval = inet_pton(AF_INET, (const char*)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }

    // Make a connection to the server
    int connectResult = connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        close(sock);
        return -1;
    }


    printf("Entering the chat...\n");
    
    // Check if performance test flag is present
    if (argc > 4) {  
        
        printf("Permormance test\n");
        char param[20] = {0};
        int paramLen = 20;
        strcat(param,COMMUNICATION_TYPE);
        strcat(param," ");
        strcat(param,PARAMETER_TYPE);


        // Perform the communication based on the communication type and parameter

        if (strcmp(COMMUNICATION_TYPE, "ipv4") == 0 && strcmp(PARAMETER_TYPE, "tcp") == 0) {

            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleIPv4TCPClient(SERVER_PORT-2);

        } else if (strcmp(COMMUNICATION_TYPE, "ipv6") == 0 && strcmp(PARAMETER_TYPE, "tcp") == 0) {
            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleIPv6TCPClient(SERVER_PORT-2);

        } else if (strcmp(COMMUNICATION_TYPE, "ipv4") == 0 && strcmp(PARAMETER_TYPE, "udp") == 0) {
            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleIPv4UDPClient(SERVER_PORT-2 , SERVER_IP_ADDRESS);

        } else if (strcmp(COMMUNICATION_TYPE, "ipv6") == 0 && strcmp(PARAMETER_TYPE, "udp") == 0) {
            
            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleIPv6UDPClient(SERVER_PORT-2 , SERVER_IP_ADDRESS);

        } else if (strcmp(COMMUNICATION_TYPE, "uds") == 0 && strcmp(PARAMETER_TYPE, "dgram") == 0) {
            
            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleUnixDgramClient();

        } else if (strcmp(COMMUNICATION_TYPE, "uds") == 0 && strcmp(PARAMETER_TYPE, "stream") == 0) {
            
            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleUnixStreamClient();

        } else if (strcmp(COMMUNICATION_TYPE, "mmap") == 0) {

            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleMmapClient((char*) PARAMETER_TYPE);

        } else if (strcmp(COMMUNICATION_TYPE, "pipe") == 0) {

            int bytesSent = send(sock,param,paramLen,0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent < 20) {
                printf("sent only %d bytes from the required %d.\n", paramLen, bytesSent);
            }  
            sleep(1);
            handleNamedPipeClient((char*)PARAMETER_TYPE);
        }

    }else{
        printf("At any moment if you wish to leave, write 'exit'.\n");
        printf("Enter message: \n");

        while (1) {
            struct pollfd pfds[2];
            pfds[0].fd = 0;    // Stdin
            pfds[0].events = POLLIN;
            pfds[1].fd = sock;
            pfds[1].events = POLLIN;

            int fd_count = 2;
        // Get input message from user
        int poll_count = poll(pfds, fd_count, 2500);
        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }
        char input[BUFFER_SIZE];
        for(int i = 0; i < fd_count; i++) 
        {
            if (pfds[i].revents & POLLIN) //if data arrived through socket/input.
            {
                if(pfds[i].fd == sock)
                {
                    // Receive response from server
                    char output[BUFFER_SIZE] = {'\0'};
                    int bytesReceived = recv(sock, output, BUFFER_SIZE, 0);
                    if (bytesReceived == -1) {
                        printf("recv() failed with error code : %d", errno);
                        break;
                    } else if (bytesReceived == 0) {
                        printf("The chat has been closed by server.\n");
                        close(sock);
                        exit(1);
                    }
                    else {
                        printf("Server: %s", output);
                    }
                }
                else if (pfds[i].fd == 0) //stdin
                {
                    if (fgets(input, BUFFER_SIZE, stdin) == NULL) 
                    {
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
                            
                            printf("Closing chat,\nBye bye...\n");
                            close(sock);
                            exit(1);
                    }
                }
            }
        }
    }
    printf("Chat been closed.\n");
    close(sock);
    return 0;
}



}
