#include "RUDP_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define FILE_SIZE 2097152 // 2MB file size in bytes
#define MAX_PACKAGE 1024  // Maximum package size

void generate_random_text_file(const char *filename, int size) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file for writing\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; ++i) {
        fputc('A' + rand() % 26, file);
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <receiver_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *receiver_ip = argv[1];
    int port = atoi(argv[2]);

    char *filename = "random_file.txt";
    generate_random_text_file(filename, FILE_SIZE);
    printf("Random file generated: %s\n", filename);

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file for reading\n");
        exit(EXIT_FAILURE);
    }

    printf("File opened for reading: %s\n", filename);

    struct rudp *sender = rudp_socket(0, 0); // Replace with appropriate values if needed
    if (sender == NULL) {
        fprintf(stderr, "Error creating RUDP socket\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    printf("RUDP socket created\n");

    // Set destination address
    if (rudp_connect(sender, receiver_ip, port) == -1) {
        fprintf(stderr, "Error setting destination address\n");
        rudp_close(sender);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    printf("Destination address set to %s:%d\n", receiver_ip, port);

    // Handshake: send SYN packet
    char syn_packet[1];
    syn_packet[0] = 'S'; // Set the first byte to 'S' for SYN flag

    printf("Sending SYN packet...\n");

    // Send SYN packet
    rudp_send(sender, syn_packet, sizeof(syn_packet));

    printf("SYN packet sent\n");

    // Receive SYN-ACK packet
    char syn_ack_packet[1];
    ssize_t bytes_received = rudp_recv(sender, syn_ack_packet);
    if (bytes_received < 0 || syn_ack_packet[0] != 'A') {
        fprintf(stderr, "Error receiving SYN-ACK packet or invalid packet received\n");
        rudp_close(sender);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    printf("Received SYN-ACK packet. Sending ACK...\n");

    // Send ACK packet
    char ack_packet[1] = {'A'};
    rudp_send(sender, ack_packet, sizeof(ack_packet));

    printf("ACK packet sent\n");

    char buffer[MAX_PACKAGE];
    size_t bytes_read;
    int total_bytes_sent = 0;
    int packet_count = 0;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // Send data packets (rudp_send is void)
        rudp_send(sender, buffer, bytes_read);
        total_bytes_sent += bytes_read;
        packet_count++;
    }

    printf("File sent via RUDP\n");
    printf("Total bytes sent: %d\n", total_bytes_sent);
    printf("Total packets sent: %d\n", packet_count);

    // Send exit message
    char exit_message[] = "EXIT";
    rudp_send(sender, exit_message, sizeof(exit_message));

    // Close the connection (implementation specific, might not be necessary)
    rudp_close(sender);

    fclose(file);

    return 0;
}
