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
#include <time.h>  

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 8888
#define ID1 6159
#define ID2 2175
char* timestamp() {
    char buffer[16];
    time_t now = time(NULL);
    struct tm* timest = localtime(&now);

    strftime(buffer, sizeof(buffer), "(%H:%M:%S)", timest);

    char * time = buffer;

    return time;
}

/*
 * Function:  socketSetup
 * --------------------
 * Setup the socket itself (bind, listen, etc.).
 *
 *  serverAddress: a sockaddr_in struct that contains all the infromation
 *                  needed to connect to the receiver end.
 *
 *  returns: socket file descriptor if successed,
 *           exit error 1 on fail.
 */
int createSocket(struct sockaddr_in *serverAddress) {
    int socketfd = -1, canReused = 1;

    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_addr.s_addr = INADDR_ANY;
    serverAddress->sin_port = htons(SERVER_PORT);

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

    //changeCCAlgorithm(socketfd, 0);

    return socketfd;
}

/*
 * Function:  getDataFromClient
 * --------------------
 * Get data from sender.
 *
 *  clientSocket: client's sock file descriptor.
 * 
 *  buffer: the buffer of data.
 * 
 *  len: buffer size.
 *
 *  returns: total bytes received,
 *           exit error 1 on fail.
 */
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

/*
 * Function:  sendData
 * --------------------
 * Sends data to sender.
 *
 *  clientSocket: client's sock file descriptor.
 * 
 *  buffer: the buffer of data.
 * 
 *  len: buffer size.
 *
 *  returns: total bytes sent,
 *           exit error 1 on fail.
 */
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

/*
 * Function:  changeCCAlgorithm
 * --------------------
 *  Change the TCP Congestion Control algorithm (reno or cubic). 
 *
 *  socketfd: socket file descriptor.
 * 
 *  whichOne: 1 for reno, 0 for cubic.
 */
void changeCCAlgorithm(int socketfd, int whichOne) {
    printf("Changing congestion control algorithm to %s...\n", (whichOne ? "reno":"cubic"));

    if (whichOne)
    {
        socklen_t CC_reno_len = strlen("reno");

        if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, "reno", CC_reno_len) != 0)
        {
            perror("setsockopt");
            exit(1);
        }
    }

    else
    {
        socklen_t CC_cubic_len = strlen("cubic");

        if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, "cubic", CC_cubic_len) != 0)
        {
            perror("setsockopt");
            exit(1);
        }
    }
}

/*
 * Function:  syncTimes
 * --------------------
 * A function that helps with timers and packet retransmission synchronization.
 *
 *  socketfd: socket file descriptor
 *  
 *  loss: are we in loss mode or timer sync mode.
 *
 *  returns: total bytes sent (always 1) or received 
 *           (1 for normal operation, 0 if the socket was closed),
 *           exit error 1 on fail.
 */
int syncTimes(int socketfd, bool loss) {
    static char dummy = 0;
    static int ret = 0;

    if (loss)
    {
        ret = send(socketfd, &dummy, sizeof(char), 0);

        if (ret == -1)
        {
            perror("send");
            exit(1);
        }

        return ret;
    }

    printf("Waiting for OK signal from sender...\n");
    ret = recv(socketfd, &dummy, sizeof(char), 0);

    if (ret == -1)
    {
        perror("recv");
        exit(1);
    }

    printf("OK – Continue\n");

    return ret;
}

/*
 * Function:  authCheck
 * --------------------
 * Makes an authentication check with the sender.
 *
 *  socketfd: socket file descriptor.
 *
 *  returns: 1 always
 */
int authCheck(int clientSocket) {
    char auth[5];

    printf("Sending authentication...\n");
    sprintf(auth, "%d", (ID1^ID2));
    sendData(clientSocket, &auth, sizeof(auth));
    printf("Authentication sent.\n");

    return 1;
}

/*
 * Function:  calculateTimes
 * --------------------
 * Makes an authentication check with the sender.
 *
 *  firstPart: a pointer that holds the array of all the receive
 *                  time of the first part of the file.
 * 
 *  secondPart: a pointer that holds the array of all the receive
 *                  time of the second part of the file.
 * 
 *  times_runned: number of times we sent the file.
 * 
 *  fileSize: size in bytes of the file that was sent.
 */
void calculateTimes(double* firstPart, double* secondPart, int times_runned, int fileSize) {
    double sumFirstPart = 0, sumSecondPart = 0, avgFirstPart = 0, avgSecondPart = 0;
    double bitrate;

    printf("--------------------------------------------\n");
    printf("(*) Times summary:\n\n");
    printf("(*) Cubic CC:\n");

    for (int i = 0; i < times_runned; ++i)
    {
        sumFirstPart += firstPart[i];
        printf("(*) Run %d, Time: %0.3lf ms\n", (i+1), firstPart[i]);
    }

    printf("\n(*) Reno CC:\n");

    for (int i = 0; i < times_runned; ++i)
    {
        sumSecondPart += secondPart[i];
        printf("(*) Run %d, Time: %0.3lf ms\n", (i+1), secondPart[i]);
    }

    if (times_runned > 0)
    {
        avgFirstPart = (sumFirstPart / (double)times_runned);
        avgSecondPart = (sumSecondPart / (double)times_runned);
    }

    bitrate = ((fileSize / 1024) / ((avgFirstPart + avgSecondPart) / 1000));

    printf("\n(*) Time avarages:\n");
    printf("(*) First part (Cubic CC): %0.3lf ms\n", avgFirstPart);
    printf("(*) Second part (Reno CC): %0.3lf ms\n", avgSecondPart);
    printf("(*) Avarage transfer time for whole file: %0.3lf ms\n", (avgFirstPart+avgSecondPart));
    printf("(*) Avarage transfer bitrate: %0.3lf KiB/s (%0.3lf MiB/s)\n", bitrate, (bitrate / 1024));
    printf("--------------------------------------------\n");
}

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

    bool whichPart = false;
    double *firstPart = (double*) malloc(currentSize * sizeof(double));
    double *secondPart = (double*) malloc(currentSize * sizeof(double));

    char* buffer = NULL;

    printf("Server setting up...\n");
    socketfd = createSocket(&serverAddress);

    printf("Listening on %s:%d. \n", SERVER_IP_ADDRESS, SERVER_PORT);

    memset(&clientAddress,0,sizeof(clientAddress));

    clientAddressLen = sizeof(clientAddress);

    clientSocket = accept(socketfd, (struct sockaddr*) &clientAddress, &clientAddressLen);

    if(clientSocket == -1){
        perror("Problem accepting.");
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientAddr, INET6_ADDRSTRLEN);

    printf("Connected to %s:%d\n", clientAddr, clientAddress.sin_port);

    getData(clientSocket, &filesize, sizeof(int));

    if (buffer == NULL)
    {
        perror("malloc");
        exit(1);
    }

        while (true)
    {
        int BytesReceived;
        
        if (!received)
        {
            if (!whichPart)
                memset(buffer, 0, filesize);

            printf("Waiting for sender data...\n");
            gettimeofday(&tv_start, NULL);
        }

        BytesReceived = getData(clientSocket, buffer + (received + (whichPart ? ((filesize/2) - 1):0)), (filesize/2) - received);
        received += BytesReceived;

        if (!BytesReceived)
            break;

        else if (received == (filesize/2))
        {
            if (!whichPart)
            {
                gettimeofday(&tv_end, NULL);
                firstPart[times] = ((tv_end.tv_sec - tv_start.tv_sec)*1000) + (((double)(tv_end.tv_usec - tv_start.tv_usec))/1000);

                printf("Received total %d bytes.\n", received);
                printf("First part received.\n");

                authCheck(clientSocket);
            }

            else
            {
                gettimeofday(&tv_end, NULL);
                secondPart[times] = ((tv_end.tv_sec - tv_start.tv_sec)*1000) + (((double)(tv_end.tv_usec - tv_start.tv_usec))/1000);

                if (++times >= currentSize)
                {
                    currentSize += 5;

                    double* p1 = (double*) realloc(firstPart, (currentSize * sizeof(double)));
                    double* p2 = (double*) realloc(secondPart, (currentSize * sizeof(double)));

                    // Safe fail so we don't lose the pointers
                    if (p1 == NULL || p2 == NULL)
                    {
                        perror("realloc");
                        exit(1);
                    }

                    firstPart = p1;
                    secondPart = p2;
                }

                printf("Received total %d bytes.\n", received);
                printf("Second part received.\n");

                syncTimes(clientSocket, true);

                printf("Waiting for sender decision...\n");
                    
                if (!syncTimes(clientSocket, false))
                    break;

                syncTimes(clientSocket, true);
            }

            whichPart = (whichPart ? false:true);

            changeCCAlgorithm(socketfd, ((whichPart) ? 1:0));

            received = 0;
        }
    }

    close(clientSocket);
    printf("Connection with {%s:%d} closed.\n", clientAddr, clientAddress.sin_port);

    calculateTimes(firstPart, secondPart, times, filesize);

    usleep(1000);

    printf("Closing socket...\n");
    close(socketfd);

    printf("Memory cleanup...\n");
    free(firstPart);
    free(secondPart);
    free(buffer);

    printf("Receiver exit.\n");
    
    return 0;
}

