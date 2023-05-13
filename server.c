#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>


#define SERVER_PORT atoi(argv[1]) 
#define BUFFER_SIZE 6400
#define MAX_LENGTH 1000
#define MAX_PACKET_SIZE_UDP 64000
#define MAX_PACKET_SIZE_UDS_DGRAM 212500
#define CHUNK_SIZE (100 * 1024 * 1024)
#define SOCKET_PATH "/tmp/my_unix_socket"
int listeningSocket;

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

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size)
    {
        *fd_size *= 2; // Double it

        pfds = realloc(*pfds, sizeof(*pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}

void handleIPv4TCPConnection(int port, int p_flag) {

    // Handle IPv4 TCP server communication
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen;

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        if (p_flag <= 1) perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Reuse the address
    int enableReuse = 1;
    int ret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        if (p_flag <= 1) printf("setsockopt() failed with error code: %d", errno);
        return;
    }

    // Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        if (p_flag <= 1) perror("Server socket binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 1) < 0) {
        if (p_flag <= 1) perror("Server socket listening failed");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connection
    clientAddrLen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        if (p_flag <= 1) perror("Server socket accept failed");
        exit(EXIT_FAILURE);
    }

    if (p_flag <= 1) printf("Connection established with the client!\n");

    clock_t startTime = clock(); // Start clock

    // Receive data from the client
    char* data = malloc(CHUNK_SIZE);
    size_t receivedBytes = 0;
    while (receivedBytes < CHUNK_SIZE) {
        ssize_t bytes = recv(clientSocket, data + receivedBytes, BUFFER_SIZE, 0);
        if (bytes < 0) {
            perror("Receiving failed");
            exit(EXIT_FAILURE);
        }
        receivedBytes += bytes;
    }

    clock_t endTime = clock(); // End clock

    // Receive checksum from the client
    unsigned int networkChecksum;
    recv(clientSocket, &networkChecksum, sizeof(unsigned int), 0);
    unsigned int receivedChecksum = ntohs(networkChecksum);

    // Calculate checksum of received data
    unsigned int calculatedChecksum = calculate_checksum(data, CHUNK_SIZE);

    // Compare checksums
    if (calculatedChecksum == receivedChecksum) {
        printf("Checksum validation passed.\n");
    } else {
        printf("Checksum validation failed.\n");
    }

    double transferTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000;

    if (p_flag <= 1) printf("Data received successfully. Total bytes received: %zu\n", receivedBytes);
    printf("ipv4_tcp,%.3lf\n", transferTime);

    // Close the sockets when finished
    close(clientSocket);
    close(serverSocket);
}


void handleIPv6TCPConnection(int port,int p_flag) {
    // Handle IPv6 TCP server communication
    int serverSocket, clientSocket;
    struct sockaddr_in6 serverAddr, clientAddr;
    socklen_t clientAddrLen;

    // Create socket
    serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        if(p_flag <= 1) perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Reuse the address if the server socket was closed
    int enableReuse = 1;
    int ret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        if(p_flag <= 1) printf("setsockopt() failed with error code: %d\n", errno);
        return;
    }

    // Set server address
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(port);
    serverAddr.sin6_addr = in6addr_any;

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        if(p_flag <= 1) perror("Server socket binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 1) < 0) {
        if(p_flag <= 1) perror("Server socket listening failed");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connection
    clientAddrLen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        if(p_flag <= 1) perror("Server socket accept failed");
        exit(EXIT_FAILURE);
    }

    if(p_flag <= 1) printf("Connection established with the client!\n");

    // Receive data
    const size_t bufferSize = 1024;  // 1KB buffer size
    char buffer[bufferSize];
    ssize_t bytesRead;
    size_t totalBytesReceived = 0;

    clock_t startTime = clock(); // Start clock

    while ((bytesRead = recv(clientSocket, buffer, bufferSize, 0)) > 0) {
        totalBytesReceived += bytesRead;

        // Clear the buffer for the next read
        memset(buffer, 0, bufferSize);
    }

    clock_t endTime = clock(); // End clock

    double transferTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000;

    if(p_flag <= 1) printf("Data received successfully. Total bytes received: %zu\n", totalBytesReceived);
    printf("ipv6_tcp,%.3lf\n", transferTime);

    // Close the sockets when finished
    close(clientSocket);
    close(serverSocket);
}

void handleIPv4UDPConnection(int port,int p_flag) {

    // Handle ipv4 tcp server communication
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen;
    clientAddrLen = sizeof(clientAddr);
    // Create socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket < 0) {
        if(p_flag <= 1) perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Reuse the address if the server socket on was closed
    int enableReuse = 1;
    int ret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        if(p_flag <= 1) printf("setsockopt() failed with error code : %d", errno);
        return;
    }

    // Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); 
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        if(p_flag <= 1) perror("Server socket binding failed");
        exit(EXIT_FAILURE);
    }

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        if(p_flag <= 1) perror("Failed to set receive timeout");
        exit(EXIT_FAILURE);
    }

    if(p_flag <= 1) printf("Connection established with the client!\n");

    // Receive data
    const size_t bufferSize = MAX_PACKET_SIZE_UDP;
    char buffer[bufferSize];
    ssize_t bytesRead;
    size_t totalBytesReceived = 0;

    clock_t startTime = clock(); // Start clock
    int num = 0;
    while (1) {
        
        bytesRead = recvfrom(serverSocket, buffer, bufferSize, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                if(p_flag <= 1) printf("Receive timeout. Total bytes received: %zu\n", totalBytesReceived);
                break;
            }
            if(p_flag <= 1) perror("Receiving data failed");
            exit(EXIT_FAILURE);
        } else if (bytesRead == 0) {
            break;
        }

        totalBytesReceived += bytesRead;
        memset(&buffer, 0, sizeof(buffer));
    }
    
    clock_t endTime = clock(); // End clock
    
    double transferTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000;
    
    if (totalBytesReceived < CHUNK_SIZE)
    {
        if(p_flag <= 1) printf("Lost data. Recieved: %zu bytes. Missing: %zu bytes.\n", totalBytesReceived , CHUNK_SIZE - totalBytesReceived);
    }
    else
    {
        if(p_flag <= 1) printf("All data arrived. Total bytes received: %zu\n", totalBytesReceived);
    }

    printf("ipv4_udp,%.3lf\n",transferTime);

    // Close the sockets when finished
    close(serverSocket);
}

void handleIPv6UDPConnection(int port,int p_flag) {

    // Handle ipv6 UDP server communication
    int serverSocket;
    struct sockaddr_in6 serverAddr, clientAddr;
    socklen_t clientAddrLen;
    clientAddrLen = sizeof(clientAddr);
    // Create socket
    serverSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket < 0) {
        if(p_flag <= 1) perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Reuse the address if the server socket on was closed
    int enableReuse = 1;
    int ret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        if(p_flag <= 1) printf("setsockopt() failed with error code : %d", errno);
        return;
    }

    // Set server address
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(port);  
    serverAddr.sin6_addr = in6addr_any;

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        if(p_flag <= 1) perror("Server socket binding failed");
        exit(EXIT_FAILURE);
    }

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = 5; 
    timeout.tv_usec = 0;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        if(p_flag <= 1) perror("Failed to set receive timeout");
        exit(EXIT_FAILURE);
    }

    if(p_flag <= 1) printf("Connection established with the client!\n");

    // Receive data
    const size_t bufferSize = MAX_PACKET_SIZE_UDP;
    char buffer[bufferSize];
    ssize_t bytesRead;
    size_t totalBytesReceived = 0;

    clock_t startTime = clock(); // Start clock
    int num = 0;
    while (1) {
        
        bytesRead = recvfrom(serverSocket, buffer, bufferSize, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                if(p_flag <= 1) printf("Receive timeout. Total bytes received: %zu\n", totalBytesReceived);
                break;
            }
            if(p_flag <= 1) perror("Receiving data failed");
            exit(EXIT_FAILURE);
        } else if (bytesRead == 0) {
            break;
        }

        totalBytesReceived += bytesRead;
        memset(&buffer, 0, sizeof(buffer));
    }
    
    clock_t endTime = clock(); // End clock
    
    double transferTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000;
    
    if (totalBytesReceived < CHUNK_SIZE)
    {
        if(p_flag <= 1) printf("Lost data. Recieved: %zu bytes. Missing: %zu bytes.\n", totalBytesReceived , CHUNK_SIZE - totalBytesReceived);
    }
    else
    {
        if(p_flag <= 1) printf("All data arrived. Total bytes received: %zu\n", totalBytesReceived);
    }

    printf("ipv6_udp,%.3lf\n",transferTime);

    // Close the sockets when finished
    close(serverSocket);
}

void handleUnixStreamServer(int p_flag) {
    // Create socket
    int serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        if(p_flag <= 1) perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    struct sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, SOCKET_PATH);

    // Unlink (remove) the socket file if it exists
    unlink(SOCKET_PATH);

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        if(p_flag <= 1) perror("Server socket binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 1) < 0) {
        if(p_flag <= 1) perror("Server socket listening failed");
        exit(EXIT_FAILURE);
    }

    // Accept a client connection
    int clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket < 0) {
        perror("Client connection acceptance failed");
        exit(EXIT_FAILURE);
    }

    if(p_flag <= 1) printf("Connection established with the client!\n");

    // Receive data from the client
    char* dataBuffer = (char*)malloc(CHUNK_SIZE);
    size_t totalBytesReceived = 0;
    ssize_t bytesRead;

    clock_t startTime = clock(); // Start clock
    while (totalBytesReceived < CHUNK_SIZE) {
        bytesRead = recv(clientSocket, dataBuffer + totalBytesReceived, CHUNK_SIZE - totalBytesReceived, 0);
        if (bytesRead < 0) {
            if(p_flag <= 1) perror("Receiving data failed");
            exit(EXIT_FAILURE);
        } 
        totalBytesReceived += bytesRead;
    }
    clock_t endTime = clock(); // End clock

    double transferTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000;

    if(p_flag <= 1) printf("Data received successfully. Total bytes received: %zu\n",totalBytesReceived);
    printf("uds_stream,%.3lf\n",transferTime);

    // Close the sockets when finished
    close(clientSocket);
    close(serverSocket);
    free(dataBuffer);
}

void handleUnixDgramServer(int p_flag) {
    // Create socket
    int serverSocket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        if(p_flag <= 1) perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    struct sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, SOCKET_PATH);
    socklen_t serverAddrLen;

    // Unlink (remove) the socket file if it exists
    unlink(SOCKET_PATH);

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        if(p_flag <= 1) perror("Server socket binding failed");
        exit(EXIT_FAILURE);
    }

    if(p_flag <= 1) printf("Server is ready to receive data...\n");

    // Receive data
    const size_t bufferSize = MAX_PACKET_SIZE_UDS_DGRAM;
    char buffer[bufferSize];
    ssize_t bytesRead;
    size_t totalBytesReceived = 0;

    clock_t startTime = clock(); // Start clock
    socklen_t clientAddrLen;
    struct sockaddr_un clientAddr;

    while (totalBytesReceived < CHUNK_SIZE) {
        serverAddrLen = sizeof(serverAddr);

        bytesRead = recvfrom(serverSocket, buffer, bufferSize, 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
        if (bytesRead < 0) {
            if(p_flag <= 1) perror("Receiving data failed");
            exit(EXIT_FAILURE);
        } else if (bytesRead == 0) {
            if(p_flag <= 1) printf("All data received. Total bytes received: %zu\n", totalBytesReceived);
            break;
        }
        clientAddrLen = serverAddrLen;
        totalBytesReceived += bytesRead;
    }
    
    clock_t endTime = clock(); // End clock
    
    double transferTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000;
    
    if(p_flag <= 1) printf("Data received successfully. Total bytes received: %zu\n",totalBytesReceived);
    printf("uds_dgram,%.3lf\n",transferTime);

    // Close the sockets when finished
    close(serverSocket);
}

void handleMmapServer(char* filename,int p_flag) {

    sleep(2);
    // Open the file for reading
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        if(p_flag <= 1) perror("open");
        exit(EXIT_FAILURE);
    }
    

    // Memory map the file
    char* data = mmap(NULL, CHUNK_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Read the data
    ssize_t bytesRead = 0;
    size_t totalBytesRead = 0;

    clock_t startTime = clock(); // Start clock
    while (totalBytesRead < CHUNK_SIZE) {
        size_t remainingBytes = CHUNK_SIZE - totalBytesRead;
        size_t chunkSize = (remainingBytes < CHUNK_SIZE) ? remainingBytes : CHUNK_SIZE;
        totalBytesRead += chunkSize;
    }
    clock_t endTime = clock(); // End clock
    
    double transferTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000.0;
    
    if(p_flag <= 1) printf("Data received successfully. Total bytes red: %zu\n",totalBytesRead);
    printf("mmap,%.3lf\n",transferTime);

    if (unlink(filename) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    close(fd);
}

void handleNamedPipeServer(const char* pipeName,int p_flag) {

    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    size_t totalBytesRead = 0;

    // Open named pipe for reading
    fd = open(pipeName, O_RDONLY);
    if (fd == -1) {
        if(p_flag <= 1) perror("open");
        exit(EXIT_FAILURE);
    }

    clock_t startTime = clock();

    // Read data from the named pipe
    while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
        totalBytesRead += bytesRead;
    }

    clock_t endTime = clock();
    double receiveTime = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000.0;

    if(p_flag <= 1) printf("Data received successfully. Total bytes received: %zu\n", totalBytesRead);
    printf("pipe,%.3lf\n",receiveTime);

    // Clean up and close the named pipe
    close(fd);
}

void sigint_handler(int signum) {
    printf("Closing connection.\n");
    close(listeningSocket);
    exit(EXIT_SUCCESS);
}


int main(int argc, char const *argv[])
{
    if (signal(SIGINT, sigint_handler) == SIG_ERR) { // Catch SIGINT to ensure closing socket
        perror("signal");
        exit(EXIT_FAILURE);
    }

    int p_flag = atoi(argv[2]);

    int newfd;

    // Start off with room for 5 connections
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    // Open the listening socket
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == -1)
    {
        if(p_flag <= 1) printf("Could not create listening socket : %d", errno);
        return 1;
    }

    // Reuse the address
    int enableReuse = 1;
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        if(p_flag <= 1) printf("setsockopt() failed with error code : %d", errno);
        return 1;
    }

    // Add the listeningSocket and stdin to set
    pfds[0].fd = 0; // Stdin
    pfds[0].events = POLLIN;
    pfds[1].fd = listeningSocket;
    pfds[1].events = POLLIN;

    fd_count = 2;

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;  // any IP at this port 
    serverAddress.sin_port = htons(SERVER_PORT); 

    // Bind the socket to the port
    int bindResult = bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (bindResult == -1)
    {
        if(p_flag <= 1) printf("Bind failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    }


    if(p_flag <= 1) printf("Chat is online\n");

    // Listen
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1)
    {
        if(p_flag <= 1) printf("listen() failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    }

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen;

    if(p_flag <= 1) printf("Waiting for people to join the chat...\n");

    // if performance (-p / -p -q)
    if(p_flag == 1 || p_flag == 2){

        while(1){
            // Receive a message from client
            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);


            // Accept incoming connection
            memset(&clientAddress, 0, sizeof(clientAddress));
            clientAddressLen = sizeof(clientAddress);

            int sock = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
            if (sock == -1)
            {
                if(p_flag <= 1) perror("accept");}

            int nbytes = recv(sock, buffer, BUFFER_SIZE, 0);
            if (nbytes < 0) {
                if(p_flag <= 1) printf("Error in receiving data: %s\n", strerror(errno));
            } else if (nbytes == 0) {
                if(p_flag <= 1) printf("Server closed the connection\n");
            } else {

                if(p_flag <= 1) printf("Received communication-type and parameters from the client.\n");
                char* c_type=NULL;
                char* p_type=NULL;
                c_type = strtok(buffer," ");
                p_type = strtok(NULL," ");
                if(p_flag <= 1) printf("type : %s\n",p_type);

                if (strcmp(c_type, "ipv4") == 0 && strcmp(p_type, "tcp") == 0) {

                    if(p_flag <= 1) printf("Preparing for performance test: ipv4-tcp\n");
                    handleIPv4TCPConnection(SERVER_PORT-2,p_flag);

                } else if (strcmp(c_type, "ipv6") == 0 && strcmp(p_type, "tcp") == 0) {

                    if(p_flag <= 1) printf("Preparing for performance test: ipv6-tcp\n");
                    handleIPv6TCPConnection(SERVER_PORT-2,p_flag);

                } else if (strcmp(c_type, "ipv4") == 0 && strcmp(p_type, "udp") == 0) {

                    if(p_flag <= 1) printf("Preparing for performance test: ipv4-udp\n");
                    handleIPv4UDPConnection(SERVER_PORT-2,p_flag);

                } else if (strcmp(c_type, "ipv6") == 0 && strcmp(p_type, "udp") == 0) {
                    
                    if(p_flag <= 1) printf("Preparing for performance test: ipv6-udp\n");
                    handleIPv6UDPConnection(SERVER_PORT-2,p_flag);

                } else if (strcmp(c_type, "uds") == 0 && strcmp(p_type, "dgram") == 0) {
                    
                    if(p_flag <= 1) printf("Preparing for performance test: dgram-uds\n");
                    handleUnixDgramServer(p_flag);

                } else if (strcmp(c_type, "uds") == 0 && strcmp(p_type, "stream") == 0) {

                    if(p_flag <= 1) printf("Preparing for performance test: stream-uds\n");
                    handleUnixStreamServer(p_flag);

                } else if (strcmp(c_type, "mmap") == 0) {

                    if(p_flag <= 1) printf("Preparing for performance test: mmap\n");
                    handleMmapServer(p_type,p_flag); // p_type here is for the file name

                } else if (strcmp(c_type, "pipe") == 0) {

                    if(p_flag <= 1) printf("Preparing for performance test: pipe\n");
                    handleNamedPipeServer(p_type,p_flag); // p_type here is for the pipe name

                }
            }
        }
    }

    while (1)
    {
        // Accept and incoming connection
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);

        int poll_count = poll(pfds, fd_count, 2500);

        if (poll_count == -1)
        {
            if(p_flag <= 1) perror("poll");
            exit(1);
        }
        // Run through the existing connections looking for data to read
        for (int i = 0; i < fd_count; i++)
        {

            // Check if someone's ready to read
            if (pfds[i].revents & POLLIN)
            { 
                // Receive a message from client
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, BUFFER_SIZE);

                if (pfds[i].fd == listeningSocket)
                {

                    // If listener is ready to read, handle new connection
                    newfd = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
                    if (newfd == -1)
                    {
                        if(p_flag <= 1) perror("accept");
                    }
                    else
                    {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

                        if(p_flag <= 1) printf("New connection!\n");

                        char client_ip[INET_ADDRSTRLEN];
                        strcpy(client_ip, inet_ntoa(clientAddress.sin_addr));

                        if(p_flag <= 1) printf("Someone enter the chat from IP: %s\n", client_ip);
                        if(p_flag <= 1) printf("Enter massage: \n");
                    }
                }
                else if (pfds[i].fd == 0) // send input
                {
                    // Reply to client
                    char input[MAX_LENGTH];
                    fgets(input, MAX_LENGTH, stdin);
                    int messageLen = strlen(input) + 1;
                    if (send(newfd, input, messageLen, 0) == -1)
                    {
                        if(p_flag <= 1) perror("send");
                    }
                }
                else
                {
                    // If not the listener, we're just a regular client
                    int nbytes = recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);

                    int sender_fd = pfds[i].fd;
                    if (nbytes <= 0)
                    {
                        close(pfds[i].fd);
                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        if (strcmp((buffer), "exit") == 0 || strcmp((buffer), "Exit") == 0)
                        {
                            // Client has chose to close the chat
                            if(p_flag <= 1) printf("Client has chose to close the chat.\n");
                            if(p_flag <= 1) printf("Waiting for people to join the chat...\n");
                            close(newfd);
                            break;
                        }
                        else
                        {
                            if(p_flag <= 1) printf("Client: %s\n", buffer);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
