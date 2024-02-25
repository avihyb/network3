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

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 8888

int main(){
    struct sockaddr_in serverAddress, clientAddress;
    struct timeval tv_start, tv_end;

    socklen_t clientAddressLen;

    char clientAddr[INET_ADDRSTRLEN];

    int filesize;
    int times = 0;
    int received = 0;
    int currentSize = 5;
    int socketfd = -1;
    int clientSocket = -1;

    char* buffer = NULL;

    printf_time("Server setting up...\n");
    socketfd = createSocket(&serverAddress);

    printf_time("Listening on %s:%d. \n", SERVER_IP_ADDRESS, SERVER_PORT);

    memset(&clientAddress,0,sizeof(clientAddress));

    clientAddressLen = sizeof(clientAddress);

    clientSocket = accept(socketfd, (struct sockaddr*) &clientAddress, &clientAddressLen);

    if(clientSocket == -1){
        perror("Problem accepting.");
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientAddr, INET6_ADDRSTRLEN);

    printf_time("Connected to %s:%d", clientAddr, clientAddress.sin_port);

    getData(clientSocket, &filesize, sizeof(int));

    if (buffer == NULL)
    {
        perror("malloc");
        exit(1);
    }

}