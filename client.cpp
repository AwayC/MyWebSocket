#include <stdio.h>      // 用于 printf, perror
#include <stdlib.h>     // 用于 exit
#include <string.h>     // 用于 strlen, memset
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
    // --- 1. 创建 Socket ---
    // AF_INET: 使用IPv4地址
    // SOCK_STREAM: 使用TCP协议
    // 0: 使用默认协议
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // --- 2. 准备服务器地址结构体 ---
    memset(&server_addr, 0, sizeof(server_addr)); // 先将结构体清零
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT); // 将端口号从主机字节序转换到网络字节序

    // 将IP地址从字符串（点分十进制）转换为网络字节序的二进制格式
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // --- 3. 连接到服务器 ---
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Successfully connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // --- 4. 构造并发送HTTP GET请求 ---
    // 这是一个最简单的HTTP/1.1 GET请求
    // - "GET /": 请求根路径
    // - "Host:": HTTP/1.1 协议必需的头字段
    // - "Connection: close": 表示完成响应后服务器应关闭连接，这简化了客户端的接收逻辑
    // - "\r\n\r\n": 空行表示请求头的结束
    const char *http_request = "GET / HTTP/1.1\r\n"
                               "Host: 127.0.0.1:8081\r\n"
                               "Connection: close\r\n"
                               "\r\n";
    printf("%s", http_request);
    if (write(client_fd, http_request, strlen(http_request)) < 0) {
        perror("Failed to send request");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP request sent.\n\n");
    printf("--- Server Response ---\n");

    // --- 5. 接收并打印服务器的响应 ---
    ssize_t bytes_read;
    // 循环读取数据，直到服务器关闭连接（read返回0）或发生错误（read返回-1）
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0'; // 在读取的数据末尾添加空字符
        printf("%s", buffer);
    }

    if (bytes_read < 0) {
        perror("Read failed");
    }
    printf("\n--- End of Response ---\n");

    // --- 6. 关闭Socket ---
    close(client_fd);
    printf("Connection closed.\n");

    return 0;
}