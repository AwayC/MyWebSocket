#include <cstdio>      // 用于 printf, perror
#include <cstdlib>     // 用于 exit
#include <cstring>     // 用于 strlen, memset
#include <unistd.h>     // 用于 read, write, close
#include <sys/socket.h> // 用于 socket, connect
#include <netinet/in.h> // 用于 sockaddr_in
#include <arpa/inet.h>  // 用于 inet_pton
#include <iostream>
// 定义服务器的IP地址、端口和缓冲区大小
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8081
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    printf("http start\n");
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Successfully connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);


    const char *http_request = "GET / HTTP/1.1\r\n"
                               "Host: 127.0.0.1:8081\r\n"
                               "Connection: keep-alive\r\n"
                               "\r\n";
    printf("%s", http_request);
    if (write(client_fd, http_request, strlen(http_request)) < 0) {
        perror("Failed to send request");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP request sent.\n\n");
    printf("--- Server Response ---\n");

    ssize_t bytes_read;
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
        close(client_fd);
        break;
        // if (write(client_fd, http_request, strlen(http_request)) < 0) {
        //     perror("Failed to send request");
        //     close(client_fd);
        //     exit(EXIT_FAILURE);
        // }
    }

    if (bytes_read < 0) {
        perror("Read failed");
    }
    printf("\n--- End of Response ---\n");

    close(client_fd);
    printf("Connection closed.\n");

    return 0;
}