/* Assignment 3:
IDs: 206769986 206768731
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>  // Include the time header for time-related functions

#define FILE_SIZE 2200000 // 2.1 MB
#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 8888

const char *filename = "random_file.txt";

// Creates a text file with random characters (A-Z), atleast 2MB in size
void generate_random_file(const char *filename) {

    
    FILE *file = fopen(filename, "w"); // arg1: File name. arg2: write

    if (file == NULL) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    srand((unsigned int)time(NULL)); // Seed the random number generator with the current time

    for (int i = 0; i < FILE_SIZE; ++i) {
        char random_char = rand() % 26 + 'A';
        fprintf(file, "%c", random_char);
    }

    fclose(file);
}

char* readFile(int* size){

    FILE* file = fopen(filename, "r"); // filename, read
    char* fileData; // file's data will be stored here

    // Null file handling
    if (file == NULL) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    *size = (int) ftell(file);
    fileData = (char*)malloc(*size*sizeof(char));
    fseek(file, 0L, SEEK_SET);
    fread(fileData, sizeof(char), *size, file);
    fclose(file);
    printf("%s | Size: %d\n", filename, *size);
    return fileData;
}

int createSocket(struct sockaddr_in *serverAddress){
    int socketfd = -1;
    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET,(const char*) SERVER_IP_ADDRESS, &serverAddress->sin_addr) == -1)
    {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error");
        exit(1);
    }

    printf("Socket has been established.\n");

    //changeCCAlgorithm(socketfd, 0);

    return socketfd;
}

int sendData(int socketfd, void* buffer, int len){
    int sent = send(socketfd, buffer, len, 0);
    if(sent == -1){
        perror("Error sending the file.");
    } 
    else if(!sent)
            printf("Receiver not available for accepting requests.\n");
    else if (sent < len)
            printf("Not all data sent. Only %d out of %d\n", sent, len);
    else 
            printf("Bytes sent: %d\n", sent);

    
    return sent;
    
}

int main() {
    struct sockaddr_in serverAddress;
    char *filedata = NULL;
    int socketfd = -1; // INVALID SOCKET
    int filesize = 0;

    printf("Sender started\n");

    printf("Generating random file...\n");

    generate_random_file(filename);

    printf("Reading file...\n");

    filedata = readFile(&filesize);

    printf("Setting up socket...\n");
    socketfd = createSocket(&serverAddress);

    printf("Conncetion to %s:%d\n", SERVER_IP_ADDRESS, SERVER_PORT);

    if(connect(socketfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1){
        perror("Couldn't connect.");
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s:%d\n", SERVER_IP_ADDRESS, SERVER_PORT);

    printf("Sending %s to receiver...\n", filename);

    sendData(socketfd, &filesize, sizeof(int));

    printf("File sent.");

   

    

    

    


    // Cleanup
    
    close(socketfd);
    free(filedata);

    // Delete the generated file
    remove(filename);

    return 0;
}
