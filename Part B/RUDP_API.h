#ifndef RUDP_API_H
#define RUDP_API_H

#define MAX_PACKAGE 1024

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h> // For struct sockaddr_in
#include <sys/socket.h> // For socket-related definitions

// Struct representing RUDP flags using bitfields
struct RUDP_FLAGS {
    unsigned int SYN : 1;
    unsigned int SYN_ACK : 1;
    unsigned int ACK : 1;
    unsigned int FIN : 1;
    unsigned int FIN_ACK : 1;
    unsigned int DATA : 1;
    unsigned int NACK : 1;
};

// Forward declaration of RUDP_FLAGS struct
typedef struct RUDP_FLAGS RUDPFlags;

/*
Reliable UDP (RUDP) HEADER contains:
    Length: 2 Bytes
    Checksum: 2 Bytes
    Flags: 1 Byte
*/
typedef struct RUDP_HEADER {
    struct RUDP_FLAGS flags;
    uint32_t checksum;
    uint32_t seq_num;
} Header;

// Struct representing the RUDP socket
extern struct rudp {
    int sockfd;                 // Socket descriptor
    struct sockaddr_in receiv_addr;
    struct sockaddr_in send_addr;
    int base;
    int next;
    int window_size;
    int resends_count;
} RUDP;

// Function to create a new RUDP socket
struct rudp* rudp_socket(int sendDelay, int expiredTime);

// Function to delete a RUDP socket
void rudp_delete(struct rudp*);

// Function to connect to a remote RUDP socket (initiate handshake)
int rudp_connect(struct rudp *U, const char *host, int port);

// Function to accept an incoming RUDP connection (receive handshake)
int rudp_accept(struct rudp *U, int port);

// Function to bind the RUDP socket to a port
int rudp_bind(struct rudp *U, int port);

// Function to receive data from the RUDP socket
int rudp_recv(struct rudp *U, char buffer[MAX_PACKAGE]);

// Function to send data through the RUDP socket
void rudp_send(struct rudp *U, const char *buffer, int size);

// Function to compute checksum
unsigned short compute_checksum(const char *data, int size);

// Function to check reliability based on checksum
int check_reliability(const char *data, int size, unsigned short checksum);

// Function to close the RUDP socket
void rudp_close(struct rudp*);

#endif // RUDP_API_H
