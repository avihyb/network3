#include "RUDP_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define FILENAME "received_file.txt" // Output filename
#define FILE_SIZE 2097152            // 2MB file size in bytes
#define MAX_PACKAGE 1024             // Maximum package size

void print_statistics(double time_taken, double speed) {
    printf("----------------------------------\n");
    printf("- * Statistics * -\n");
    printf("- Run #1 Data: Time=%.1fms; Speed=%.2fMB/s\n", time_taken * 1000, speed);
    printf("-\n");
    printf("- Average time: %.1fms\n", time_taken * 1000);
    printf("- Average bandwidth: %.2fMB/s\n", speed);
    printf("----------------------------------\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);

    // Create RUDP socket
    struct rudp *receiver = rudp_socket(0, 0);
    if (receiver == NULL) {
        fprintf(stderr, "Error creating RUDP socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Starting Receiver...\n");

    // Set SO_REUSEADDR socket option
    int reuse = 1;
    if (setsockopt(receiver->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        fprintf(stderr, "Error setting SO_REUSEADDR option: %s\n", strerror(errno));
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    // Bind socket to the specified port
    if (rudp_bind(receiver, port) == -1) {
        fprintf(stderr, "Error binding RUDP socket\n");
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    printf("Receiver bound to port %d\n", port);
    printf("Waiting for RUDP connection...\n");

    // Receive SYN packet
    char syn_packet[1];
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    ssize_t bytes_received = recvfrom(receiver->sockfd, syn_packet, sizeof(syn_packet), 0,
                                      (struct sockaddr *)&sender_addr, &sender_addr_len);
    if (bytes_received == -1) {
        perror("Error receiving SYN packet");
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    // Check if the received packet is a SYN packet
    if (syn_packet[0] != 'S') {
        fprintf(stderr, "Received packet is not a SYN packet\n");
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    printf("Received SYN packet from %s:%d. Sending SYN-ACK...\n",
           inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port));

    // Send SYN-ACK packet
    char syn_ack_packet[1] = {'A'}; // Set the first byte to 'A' for SYN-ACK flag
    ssize_t bytes_sent = sendto(receiver->sockfd, syn_ack_packet, sizeof(syn_ack_packet), 0,
                                 (struct sockaddr *)&sender_addr, sender_addr_len);
    if (bytes_sent == -1) {
        perror("Error sending SYN-ACK packet");
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    printf("SYN-ACK packet sent\n");

    // Wait for ACK packet
    char ack_packet[1];
    bytes_received = recvfrom(receiver->sockfd, ack_packet, sizeof(ack_packet), 0,
                               (struct sockaddr *)&sender_addr, &sender_addr_len);
    if (bytes_received == -1) {
        perror("Error receiving ACK packet");
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    if (ack_packet[0] != 'A') {
        fprintf(stderr, "Received packet is not an ACK packet\n");
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    printf("ACK packet received. Connection established with %s:%d\n",
           inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port));

    // Receive file and measure time
    clock_t start_time = clock();
    FILE *file = fopen(FILENAME, "wb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file for writing\n");
        rudp_close(receiver);
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_PACKAGE];
    size_t bytes_written;
    while ((bytes_received = rudp_recv(receiver, buffer)) > 0) {
        bytes_written = fwrite(buffer, 1, bytes_received, file);
        if (bytes_written != bytes_received) {
            fprintf(stderr, "Error writing to file\n");
            fclose(file);
            rudp_close(receiver);
            exit(EXIT_FAILURE);
        }
        // Check for the exit message
        if (strncmp(buffer, "EXIT", 4) == 0) {
            break;
        }
    }

    if (bytes_received == -1) {
        perror("Error receiving data");
    }

    clock_t end_time = clock();
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("File transfer completed.\n");
    printf("ACK sent.\n");

    // Close file
    fclose(file);

    // Close RUDP connection
    rudp_close(receiver);

    // Print statistics
    print_statistics(time_taken, FILE_SIZE / time_taken);

    printf("Receiver end.\n");

    return 0;
}
