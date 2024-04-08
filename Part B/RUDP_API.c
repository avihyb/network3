#include "RUDP_API.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>



// Function to create a new RUDP socket
struct rudp* rudp_socket(int sendDelay, int expiredTime) {
    // Allocate memory for the rudp struct
    struct rudp* U = (struct rudp*)malloc(sizeof(struct rudp));
    if (U == NULL) {
        fprintf(stderr, "Failed to allocate memory for rudp socket\n");
        return NULL;
    }

    // Initialize socket descriptor and other fields
    U->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (U->sockfd == -1) {
        perror("Error creating socket");
        free(U);
        return NULL;
    }

    memset(&U->receiv_addr, 0, sizeof(U->receiv_addr)); // Clear receive address
    memset(&U->send_addr, 0, sizeof(U->send_addr)); // Clear send address
    U->base = 0;
    U->next = 0;
    U->window_size = 0; // Initialize to 0 or to the desired window size
    U->resends_count = 0;

    // Additional initialization steps can be added here if needed

    return U;
}




// Function to delete a RUDP socket
void rudp_delete(struct rudp* U) {
    if (U != NULL) {
        // Close the socket if it's open
        if (U->sockfd != -1) {
            close(U->sockfd);
        }
        // Free the memory allocated for the RUDP struct
        free(U);
    }
}



// Function to connect to a remote RUDP socket (initiate handshake)
int rudp_connect(struct rudp *U, const char *host, int port) {
    // Clear the send address structure
    memset(&(U->send_addr), 0, sizeof(U->send_addr));

    // Set the send address parameters
    U->send_addr.sin_family = AF_INET;
    U->send_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &U->send_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(U->sockfd);
        return -1;
    }

    return 0; // Connection initiated successfully
}

// Function to bind the RUDP socket to a port
int rudp_bind(struct rudp *U, int port) {
    memset(&(U->receiv_addr), 0, sizeof(U->receiv_addr));
    U->receiv_addr.sin_family = AF_INET;
    U->receiv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    U->receiv_addr.sin_port = htons(port);

    if (bind(U->sockfd, (struct sockaddr *)&(U->receiv_addr), sizeof(U->receiv_addr)) == -1) {
        perror("bind");
        return -1;
    }

    return 0;
}

int rudp_accept(struct rudp *U, int port) {
    // Receive SYN packet from client
    Header syn_packet;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    ssize_t bytes_received = recvfrom(U->sockfd, &syn_packet, sizeof(Header), 0, (struct sockaddr *)&(U->receiv_addr), &client_addr_len);
    if (bytes_received == -1) {
        perror("Error receiving SYN packet");
        close(U->sockfd);
        return -1;
    }

    // Check if the received packet is a SYN packet
    if (syn_packet.flags.SYN != 1) {
        fprintf(stderr, "Received packet is not a SYN packet\n");
        close(U->sockfd);
        return -1;
    }

    // Prepare and send SYN-ACK packet
    Header syn_ack_packet;
    syn_ack_packet.flags.SYN = 1;
    syn_ack_packet.flags.SYN_ACK = 1;
    syn_ack_packet.flags.ACK = 0;
    syn_ack_packet.flags.FIN = 0;
    syn_ack_packet.flags.FIN_ACK = 0;
    syn_ack_packet.flags.DATA = 0;
    syn_ack_packet.flags.NACK = 0;
    syn_ack_packet.checksum = 0; // You need to compute the checksum
    syn_ack_packet.seq_num = 0;  // Set the sequence number as needed

    ssize_t bytes_sent = sendto(U->sockfd, &syn_ack_packet, sizeof(Header), 0, (struct sockaddr *)&(U->receiv_addr), client_addr_len);
    if (bytes_sent == -1) {
        perror("Error sending SYN-ACK packet");
        close(U->sockfd);
        return -1;
    }

    // Additional steps such as waiting for ACK and handling timeouts can be added here

    return 0; // Connection accepted successfully
}



int rudp_recv(struct rudp *U, char buffer[MAX_PACKAGE]) {
    // Receive data from the RUDP socket
    ssize_t bytes_received = recvfrom(U->sockfd, buffer, MAX_PACKAGE, 0, NULL, NULL);
    if (bytes_received == -1) {
        perror("Error receiving data");
        return -1;
    }

    // Null-terminate the received data to make it a valid string
    buffer[bytes_received] = '\0';

    return bytes_received; // Return the number of bytes received
}

// Function to send data through the RUDP socket
void rudp_send(struct rudp *U, const char *buffer, int size) {
    // Check if the socket file descriptor is valid
    if (U->sockfd == -1) {
        fprintf(stderr, "Invalid socket file descriptor\n");
        return;
    }

    // Check if the destination address is properly initialized
    if (U->send_addr.sin_family == 0) {
        fprintf(stderr, "Destination address not set\n");
        return;
    }

    // Send data through the RUDP socket
    ssize_t bytes_sent = sendto(U->sockfd, buffer, size, 0, (struct sockaddr *)&(U->send_addr), sizeof(U->send_addr));
    if (bytes_sent == -1) {
        perror("Error sending data");
        return;
    }

    // Check if all data has been sent
    if (bytes_sent != size) {
        fprintf(stderr, "Not all data sent\n");
        return;
    }

    printf("Data sent successfully\n");
}

// Function to compute checksum
unsigned short compute_checksum(const char *data, int size) {
    unsigned long sum = 0;
    unsigned short *ptr = (unsigned short *)data;

    // Calculate the sum of 16-bit words in the data
    while (size > 1) {
        sum += *ptr++;
        size -= 2;
    }

    // If the data size is odd, add the last byte
    if (size == 1) {
        sum += *((unsigned char *)ptr);
    }

    // Fold the carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Take the one's complement
    return (unsigned short)(~sum);
}

// Function to check reliability based on checksum
int check_reliability(const char *data, int size, unsigned short checksum) {
    // Compute the checksum of the provided data
    unsigned short computed_checksum = compute_checksum(data, size);

    // Compare the computed checksum with the given checksum
    if (computed_checksum == checksum) {
        return 1; // Reliability check passed
    } else {
        return 0; // Reliability check failed
    }
}

// Function to close the RUDP socket
void rudp_close(struct rudp* U) {
    if (U != NULL) {
        // Close the socket associated with the RUDP connection
        if (U->sockfd != -1) {
            close(U->sockfd);
        }

        // Free the memory allocated for the struct rudp
        free(U);
    }
}
