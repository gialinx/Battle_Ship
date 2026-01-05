// network.h - Network functions
#ifndef NETWORK_H
#define NETWORK_H

#define PORT 5501

// Connect to server
int connect_to_server(const char* ip, int port);

// Send message to server
int send_to_server(int sockfd, const char* message);

// Receive message from server
int receive_from_server(int sockfd, char* buffer, int size);

#endif
