#include <cstdlib>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define SERVER_PORT 8088
#define TEST_PORT 8088
#define BUFFER_SIZE 1024

// 启动服务器子进程（实际项目中建议用 exec 启动编译后的二进制）
void start_server() {
    system("cd .. && ./http_server &"); // 后台运行服务器
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待服务器启动
}

// 发送 HTTP 请求并验证响应
bool test_http_response() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "连接服务器失败\n";
        return false;
    }

    // 发送 HTTP 请求
    const char* req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(sock, req, strlen(req), 0);

    // 接收响应
    char buffer[BUFFER_SIZE];
    ssize_t len = recv(sock, buffer, sizeof(buffer)-1, 0);
    close(sock);

    if (len <= 0) return false;
    buffer[len] = '\0';

    // 验证响应内容
    std::string response(buffer);
    return (response.find("HTTP/1.1 200 OK") != std::string::npos) &&
           (response.find("<h1>Hello, World!</h1>") != std::string::npos);
}

int main() {
    //start_server(); // 启动服务器，在主程序外启动，例如在Jenkinsfine的测试阶段启动

    bool success = test_http_response();
    if (success) {
        std::cout << "✅ 集成测试通过\n";
        return EXIT_SUCCESS;
    } else {
        std::cerr << "❌ 集成测试失败\n";
        return EXIT_FAILURE;
    }
}