#include <netinet/tcp.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define FILE_SIZE 2200000
#define MAX_BUFFER_SIZE 1024

const char *filename = "random_file.txt";

void generate_random_file(const char *filename) {
    FILE *file = fopen(filename, "w");

    if (file == NULL) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    srand((unsigned int)time(NULL));

    for (int i = 0; i < FILE_SIZE; ++i) {
        char random_char = rand() % 26 + 'A';
        fprintf(file, "%c", random_char);
    }

    fclose(file);
}

char *readFile(int *size) {
    FILE *file = fopen(filename, "r");
    char *fileData;

    if (file == NULL) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    *size = (int)ftell(file);
    fileData = (char *)malloc(*size * sizeof(char));
    fseek(file, 0L, SEEK_SET);
    fread(fileData, sizeof(char), *size, file);
    fclose(file);
    printf("%s | Size: %d Bytes\n", filename, *size);
    return fileData;
}

int createSocket(struct sockaddr_in *serverAddress, const char *receiverIP, int receiverPort) {
    int socketfd = -1;

    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET; // IPv4
    serverAddress->sin_port = htons(receiverPort);

    if (inet_pton(AF_INET, receiverIP, &serverAddress->sin_addr) == -1) {
        perror("Error converting IP address");
        exit(EXIT_FAILURE);
    }

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket has been established.\n");

    return socketfd;
}

int sendData(int socketfd, void *buffer, int len) {
    int sent = send(socketfd, buffer, len, 0);

    if (sent == -1) {
        perror("Error sending the file.");
    } else if (sent == 0) {
        printf("Receiver not available for accepting requests.\n");
    } else if (sent < len) {
        printf("Not all data sent. Only %d out of %d\n", sent, len);
    }

    return sent;
}

void setCongestionControl(int socketfd, const char *ALGO) {
    if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, ALGO, strlen(ALGO) + 1) == -1) {
        perror("Error setting congestion control algorithm");
        exit(EXIT_FAILURE);
    }

    printf("CC Algorithm set to: %s\n", ALGO);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serverAddress;
    char *filedata = NULL;
    int socketfd = -1;
    int filesize = 0;
    char *ALGO = NULL;
    int chunkSize = MAX_BUFFER_SIZE;
    int remainingBytes;

    char *receiverIP = NULL;
    int receiverPort = 0;

    for (int i = 1; i < argc; i++) {  // Skip argv[0] which is the program name
        if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
            receiverIP = argv[i + 1];
            i++;  // Skip the value
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            receiverPort = atoi(argv[i + 1]);
            i++;  // Skip the value
        } else if (strcmp(argv[i], "-algo") == 0 && i + 1 < argc) {
            ALGO = argv[i + 1];
            i++;  // Skip the value
        } else {
            printf("Invalid argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (receiverPort == 0 || receiverIP == NULL || ALGO == NULL) {
        fprintf(stderr, "Both -ip RECEIVER_IP -p RECEIVER_PORT -a ALGORITHM are required.\n");
        exit(EXIT_FAILURE);
    }
    printf("ALGO: %s\n", ALGO);
    printf("Sender started\n");

    printf("Generating random file...\n");

    generate_random_file(filename);

    printf("Reading file...\n");

    filedata = readFile(&filesize);

    printf("Setting up socket...\n");
    socketfd = createSocket(&serverAddress, receiverIP, receiverPort);

    printf("Connection to %s:%d\n", receiverIP, receiverPort);

    if (connect(socketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Couldn't connect.");
        exit(EXIT_FAILURE);
    }

    setCongestionControl(socketfd, ALGO);

    printf("Connected to %s:%d\n", receiverIP, receiverPort);

    printf("Sending %s to receiver...\n", filename);

    sendData(socketfd, &filesize, sizeof(int));

    remainingBytes = filesize;

    int totalSentBytes = 0;

    while (remainingBytes > 0) {
        int bytesToSend = (remainingBytes < chunkSize) ? remainingBytes : chunkSize;

        int sentBytes = sendData(socketfd, filedata + (filesize - remainingBytes), bytesToSend);

        totalSentBytes += sentBytes;
        remainingBytes -= sentBytes;
    }

    printf("send again? (1/0)\n");

    int choice;
    scanf("%d", &choice);
    int resendSignal = 'y';
    if (choice == 1) {
        
        sendData(socketfd, &resendSignal, sizeof(int));
    } else if (choice == 0){
        resendSignal = 'n';
        sendData(socketfd, &resendSignal, sizeof(int));
    }

    // Cleanup
    close(socketfd);
    free(filedata);

    // Delete the generated file
    remove(filename);

    return 0;
}
