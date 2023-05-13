# Chat Tool & Network Performance Test

This README provides information about the chat tool implementation for the assignment, including its purpose, functionality, and usage instructions.

## Purpose

The purpose of this comunication tool is to implement a network communication system that allows two sides to send and receive messages simultaneously. Additionally, the tool should be extended to serve as a network performance test utility, capable of transmitting data using different communication styles and measuring the time it takes.

## Functionality

The chat tool consists of two parts: basic functionality (Part A) and performance testing (Part B).

## Part A: Basic Functionality

The basic functionality of the chat tool allows two sides to communicate over the network. The tool should be executed with the following command-line arguments:

**Client Side:** `stnc -c IP PORT`

**Server Side:** `stnc -s PORT`

The communication will be performed using IPv4 TCP protocol. The chat tool acts as a command-line interface that reads input from the keyboard and listens for messages from the other side due to using poll function.

In order to make the chat simultansily and synchronized in Server side as Client side , we use poll mechanishem.

### Poll in Server
The server create set of fd , first enter stdin and then enter listener socket.
Stdin - will turn on inside the set when The server try to send message to the client.
Listener - will turn on inside the set when some client try to connect the chat tool.

When Listener is turn on , we will add new Client to the set.
Client - will turn on inside the set when The client send message to Server.

### Poll in Client
The client create set of fd , first enter stdin , when connect to the server will add the socket through he connected.
Stdin - will turn on inside the set when client try to send message to Server.
Socket - will turn on inside the set when Server try to send message to Client.

## Part B: Performance Test

The performance testing extends the chat tool to work as a network performance test utility. 
The test will based on sending data as size of 100MB from client to server , the server will measure taken time and print it to screen.
The following communication styles are supported:

- TCP/UDP IPv4/IPv6 (4 variants)
- Memory-mapped file (mmap) and Named pipe (2 variants)
- Unix Domain Socket (UDS) - Stream and Datagram (2 variants)

The usage for the client side is as follows: 

**Client Side:** `stnc -c IP PORT -p <type> <param>`

The `-p` flag indicates that a performance test should be performed. `<type>` represents the communication type, and `<param>` is a parameter for the type. The communication types can be `ipv4`, `ipv6`, `mmap`, `pipe`, or `uds`, depending on the desired communication style.

The usage for the server side is as follows:

**Server Side:** `stnc -s PORT -p -q`

The `-p` flag indicates a performance test, and the `-q` flag enables quiet mode, where only the testing results are printed. This mode is essential for automatic testing.

The results of the performance test are reported in milliseconds (ms) and printed in the format `name_type,time`.

### Type&paramter:
In total there is 8 possible input of type and paramter : 
1. **IPv4 TCP**: This option utilizes the IPv4 protocol and the TCP transport layer protocol.

   Example usage:
   ```
   ./stnc -s 8080 -p 
   ./stnc -c 127.0.0.1 8080 -p ipv4 tcp
   ```

2. **IPv4 UDP**: This option uses the IPv4 protocol and the UDP transport layer protocol.

   Example usage:
   ```
   ./stnc -s 8080 -p 
   ./stnc -c 127.0.0.1 8080 -p ipv4 udp
   ```

3. **IPv6 TCP**: This option utilizes the IPv6 protocol and the TCP transport layer protocol.

   Example usage:
   ```
   ./stnc -s 8080 -p 
   ./stnc -c 127.0.0.1 8080 -p ipv6 tcp
   ```

4. **IPv6 UDP**: This option uses the IPv6 protocol and the UDP transport layer protocol.

   Example usage:
   ```
   ./stnc -s 8080 -p 
   ./stnc -c 127.0.0.1 8080 -p ipv6 udp
   ```

5. **UDS DGRAM**: This option utilizes Unix Domain Sockets with the Datagram protocol.

   Example usage:
   ```
   ./stnc -s 8080 -p 
   ./stnc -c 127.0.0.1 8080 -p uds dgram
   ```

6. **UDS STREAM**: This option utilizes Unix Domain Sockets with the Stream protocol.

   Example usage:
   ```
   ./stnc -s 8080 -p 
   ./stnc -c 127.0.0.1 8080 -p uds stream
   ```

7. **NMAP FILENAME**: This option uses Memory Mapped Files for communication. It measures the performance of data exchange through a memory-mapped file shared between the client and server.

   Example usage:
   ```
   ./stnc -s 8080 -p
   ./stnc -c 127.0.0.1 8080 -p mmap file.txt
   ```

8. **PIPE FILENAME**: This option uses named pipes for communication. It measures the performance of data exchange through a named pipe shared between the client and server.

   Example usage:
   ```
   ./stnc -s 8080
   ./stnc -c 127.0.0.1 8080 -p pipe file.txt
   ```
   
  ## Stnc file
  
  Stnc file is the shell of the tool , he manage to decide which flag i am using , Am i gonna run Server/Client and if it will be in Performance mode or/and Quite mode.
  In order to run client and server at the same time he create 2 process one for client and one for server with the needed info to run as needed.
  
  
  
