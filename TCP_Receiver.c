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

char *timestamp()
{
    char *timeStr = malloc(16);
    time_t now = time(NULL);
    struct tm *timest = localtime(&now);

    strftime(timeStr, 16, "(%H:%M:%S)", timest);

    return timeStr;
}

int createSocket(struct sockaddr_in *serverAddress, int port)
{
    int socketfd = -1, canReused = 1;

    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_addr.s_addr = INADDR_ANY;
    serverAddress->sin_port = htons(port);

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &canReused, sizeof(canReused)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(socketfd, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(socketfd, 1) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Socket successfully created.\n");

    return socketfd;
}

int getData(int clientSocket, void *buffer, int len)
{
    int recvb = recv(clientSocket, buffer, len, 0);

    if (recvb == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    else if (recvb == 0)
    {
        printf("Connection with client closed.\n");
        return 0;
    }

    return recvb;
}

int sendData(int clientSocket, void *buffer, int len)
{
    int sentd = send(clientSocket, buffer, len, 0);

    if (sentd == -1)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }

    return sentd;
}

void receiveFile(int clientSocket, int filesize)
{
    FILE *file = fopen("received_file.txt", "w");

    if (file == NULL)
    {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    int remainingBytes = filesize;
    char buffer[MAX_BUFFER_SIZE];

    while (remainingBytes > 0)
    {
        int bytesRead = getData(clientSocket, buffer, sizeof(buffer));

        if (bytesRead == 0)
        {
            printf("Connection closed by the sender.\n");
            fclose(file);
            exit(EXIT_SUCCESS);
        }
        else if (bytesRead < 0)
        {
            perror("Error receiving file");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        fwrite(buffer, 1, bytesRead, file);
        remainingBytes -= bytesRead;
    }

    fclose(file);
}

int handleSenderResponse(int clientSocket)
{
    char response[MAX_BUFFER_SIZE];
    getData(clientSocket, response, sizeof(response));

    if (strcmp(response, "exit") == 0)
    {
        printf("Received exit message from sender. Closing connection.\n");
        return 0; // Exit the loop
    }
    else if (strcmp(response, "resend") == 0)
    {
        printf("Received resend message from sender. Receiving file again.\n");
        return 1; // Continue to receive the file again
    }
    else
    {
        printf("Received unknown response from sender: %s\n", response);
        return 0; // Exit the loop
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-algo") != 0)
    {
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

    struct timeval startTime, endTime;
    double elapsedTime;

    double totalElapsedTime = 0;
    double totalBandwidth = 0;
    int totalFilesReceived = 0;

    printf("Server setting up...\n");
    socketfd = createSocket(&serverAddress, port);

    printf("Listening on port %d.\n", port);

    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLen = sizeof(clientAddress);

    clientSocket = accept(socketfd, (struct sockaddr *)&clientAddress, &clientAddressLen);

    if (clientSocket == -1)
    {
        perror("Problem accepting.");
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientAddr, INET6_ADDRSTRLEN);

    printf("Connected to %s:%d\n", clientAddr, clientAddress.sin_port);

    while (1)
    {
        gettimeofday(&startTime, NULL);

        getData(clientSocket, &filesize, sizeof(int));

        // Assuming you have a function receiveFile to handle receiving the file
        // This is a placeholder and you need to implement it
        receiveFile(clientSocket, filesize);

        gettimeofday(&endTime, NULL);
        elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000.0; // Convert to milliseconds
        elapsedTime += (endTime.tv_usec - startTime.tv_usec) / 1000.0;

        printf("Time taken to receive file: %.2f milliseconds\n", elapsedTime);

        // Calculate and print bandwidth
        double bandwidth = (filesize / 1024.0) / (elapsedTime / 1000.0); // in KB/s
        printf("Bandwidth: %.2f KB/s\n", bandwidth);

        totalElapsedTime += elapsedTime;
        totalBandwidth += bandwidth;
        totalFilesReceived++;

        // Assuming you have a function handleSenderResponse to handle sender responses
        // This is a placeholder and you need to implement it
        if (!handleSenderResponse(clientSocket))
        {
            break;
        }
    }

    // Calculate and print average time and total average bandwidth
    double averageTime = totalElapsedTime / totalFilesReceived;
    double averageBandwidth = totalBandwidth / totalFilesReceived;

    printf("Total Files Received: %d\n", totalFilesReceived);
    printf("Average Time: %.2f milliseconds\n", averageTime);
    printf("Total Average Bandwidth: %.2f KB/s\n", averageBandwidth);

    printf("Closing connection with {%s:%d}.\n", clientAddr, clientAddress.sin_port);
    close(clientSocket);

    printf("Closing socket...\n");
    close(socketfd);

    printf("Receiver exit.\n");

    return 0;
}
