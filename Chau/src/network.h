// network.h - Tách logic network để dùng chung
#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define PORT 5500
#define BUFF_SIZE 8192
#define MAP_SIZE 13

// Kết nối tới server
int connect_to_server(const char *ip, int port);

// Gửi lệnh tới server
void send_command(int sockfd, const char *cmd);

// Nhận dữ liệu từ server (non-blocking)
int receive_data(int sockfd, char *buffer, int size);

#endif
