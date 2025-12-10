// network.c - Implementation của network logic
#include "network.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int connect_to_server(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    // Set socket receive timeout instead of non-blocking
    // This prevents hanging but still receives all data
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000; // 50ms timeout - longer to ensure we catch messages
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return sockfd;
}

void send_command(int sockfd, const char *cmd) {
    char msg[BUFF_SIZE];
    snprintf(msg, sizeof(msg), "%s#", cmd);
    send(sockfd, msg, strlen(msg), 0);
}

int receive_data(int sockfd, char *buffer, int size) {
    return recv(sockfd, buffer, size - 1, 0);
}
