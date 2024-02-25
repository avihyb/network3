#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>

#define MAX_BUFFER_SIZE 1024

char* timestamp() {
    char buffer[16];
    time_t now = time(NULL);
    struct tm* timest = localtime(&now);

    strftime(buffer, sizeof(buffer), "(%H:%M:%S)", timest);

    char * time = buffer;

    return time;
}

int createSocket(struct sockaddr_in *serverAddress, int port) {
    int socketfd = -1, canReused = 1;

    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_addr.s_addr = INADDR_ANY;
    serverAddress->sin_port = htons(port);

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &canReused, sizeof(canReused)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    if (bind(socketfd, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(socketfd, 1) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("Socket successfully created.\n");

    // Uncomment the line below if you have a function called changeCCAlgorithm
    // changeCCAlgorithm(socketfd, 0);

    return socketfd;
}

int getData(int clientSocket, void *buffer, int len) {
    int recvb = recv(clientSocket, buffer, len, 0);

    if (recvb == -1)
    {
        perror("recv");
        exit(1);
    }

    else if (!recvb)
    {
        printf("Connection with client closed.\n");
        return 0;
    }

    return recvb;
}

int sendData(int clientSocket, void* buffer, int len) {
    int sentd = send(clientSocket, buffer, len, 0);

    if (sentd == 0)
    {
        printf("Sender doesn't accept requests.\n");
    }

    else if (sentd < len)
    {
        printf("Data was only partly send (%d/%d bytes).\n", sentd, len);
    }

    else
    {
        printf("Total bytes sent is %d.\n", sentd);
    }

    return sentd;
}

int main(int argc, char *argv[]) {
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-algo") != 0) {
        printf("Usage: %s -p <port> -algo <ALGO>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);
    

    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLen;
    char clientAddr[INET_ADDRSTRLEN];

    int filesize;
    int socketfd = -1;
    int clientSocket = -1;


    printf("Server setting up...\n");
    socketfd = createSocket(&serverAddress, port);

    printf("Listening on port %d.\n", port);

    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLen = sizeof(clientAddress);

    clientSocket = accept(socketfd, (struct sockaddr*) &clientAddress, &clientAddressLen);

    if(clientSocket == -1){
        perror("Problem accepting.");
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientAddr, INET6_ADDRSTRLEN);

    printf("Connected to %s:%d\n", clientAddr, clientAddress.sin_port);

    getData(clientSocket, &filesize, sizeof(int));

    // Add your logic here to handle the received data

    close(clientSocket);
    printf("Connection with {%s:%d} closed.\n", clientAddr, clientAddress.sin_port);

    usleep(1000);

    printf("Closing socket...\n");
    close(socketfd);

    printf("Receiver exit.\n");

    return 0;
}
