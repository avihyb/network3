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
    printf("%s | Size: %d\n", filename, *size);
    return fileData;
}

int createSocket(struct sockaddr_in *serverAddress, const char *receiverIP, int receiverPort) {
    int socketfd = -1;

    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET;
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
    } else if (!sent) {
        printf("Receiver not available for accepting requests.\n");
    } else if (sent < len) {
        printf("Not all data sent. Only %d out of %d\n", sent, len);
    } else {
        printf("Bytes sent: %d\n", sent);
    }

    return sent;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serverAddress;
    char *filedata = NULL;
    int socketfd = -1;
    int filesize = 0;
    char *ALGO = NULL;
    int chunkSize = MAX_BUFFER_SIZE;
    int remainingBytes = filesize;
    int offset = 0;

    char *receiverIP = NULL;
    int receiverPort = 0;

    int opt;

    while ((opt = getopt(argc, argv, "ip:algo:")) != -1) {
        switch (opt) {
        case 'i':
            receiverIP = optarg;
            break;
        case 'p':
            receiverPort = atoi(optarg);
            break;
        case 'algo':
            ALGO = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s -ip RECEIVER_IP -p RECEIVER_PORT -algo ALGORITHM\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (receiverPort == 0 || receiverIP == NULL || ALGO == NULL) {
        fprintf(stderr, "Both -ip RECEIVER_IP -p RECEIVER_PORT -algo ALGORITHM are required.\n");
        exit(EXIT_FAILURE);
    }

    if (ALGO != NULL) {
        socketfd = socket(AF_INET, SOCK_STREAM, 0);

        if (socketfd == -1) {
            perror("Error creating socket");
            exit(EXIT_FAILURE);
        }

        if (strcmp(ALGO, "reno") == 0) {
            setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, "reno", sizeof("reno"));
        } else if (strcmp(ALGO, "cubic") == 0) {
            setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, "cubic", sizeof("cubic"));
        } else {
            fprintf(stderr, "Invalid algorithm. Supported algorithms: reno, cubic.\n");
            close(socketfd);
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Congestion control algorithm (-algo) is required.\n");
        exit(EXIT_FAILURE);
    }

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

    printf("Connected to %s:%d\n", receiverIP, receiverPort);

    printf("Sending %s to receiver...\n", filename);

    sendData(socketfd, &filesize, sizeof(int));

    while (remainingBytes > 0) {
        int bytesToSend = (remainingBytes < chunkSize) ? remainingBytes : chunkSize;

        int sentBytes = sendData(socketfd, filedata + offset, bytesToSend);

        offset += sentBytes;
        remainingBytes -= sentBytes;
    }

    // Cleanup
    close(socketfd);
    free(filedata);

    // Delete the generated file
    remove(filename);

    return 0;
}
