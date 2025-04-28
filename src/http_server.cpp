#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 简单的 HTTP 响应
const std::string http_response = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n\r\n"
"<html><body><h1>Hello, World!</h1></body></html>";

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    
    // 读取客户端请求
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        std::cerr << "Error receiving data from client\n";
        return;
    }
    
    buffer[bytes_received] = '\0';  // 确保字符串结束
    std::cout << "Request received:\n" << buffer << "\n";

    // 发送 HTTP 响应
    send(client_socket, http_response.c_str(), http_response.length(), 0);

    // 关闭客户端连接
    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    // 创建服务器套接字
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有接口
    server_addr.sin_port = htons(PORT);  // 设置端口号

    // 绑定套接字到指定地址
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed\n";
        close(server_socket);
        return 1;
    }

    // 开始监听
    if (listen(server_socket, 10) == -1) {
        std::cerr << "Listen failed\n";
        close(server_socket);
        return 1;
    }

    std::cout << "HTTP server is running on port " << PORT << "...\n";

    // 主循环，接受并处理客户端请求
    while (true) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        std::cout << "Client connected\n";
        handle_client(client_socket);
    }

    // 关闭服务器套接字
    close(server_socket);
    return 0;
}
