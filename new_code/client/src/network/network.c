// network.c - Network functions implementation
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Connect to server
int connect_to_server(const char* ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    printf("Connected to server at %s:%d\n", ip, port);
    return sockfd;
}

// Send message to server
int send_to_server(int sockfd, const char* message) {
    int len = strlen(message);
    int sent = send(sockfd, message, len, 0);

    if (sent < 0) {
        perror("Send failed");
        return -1;
    }

    return sent;
}

// Receive message from server
int receive_from_server(int sockfd, char* buffer, int size) {
    int received = recv(sockfd, buffer, size - 1, 0);

    if (received < 0) {
        perror("Receive failed");
        return -1;
    }

    buffer[received] = '\0';  // Null-terminate
    return received;
}
