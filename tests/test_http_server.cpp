#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TEST_PORT 8081
#define TEST_BUFFER_SIZE 1024

void start_test_server() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(TEST_PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 1);
    
    int client_socket = accept(server_socket, NULL, NULL);
    const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, Test!";
    send(client_socket, response, strlen(response), 0);
    
    close(client_socket);
    close(server_socket);
}

void test_http_client() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TEST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    const char* request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(sock, request, strlen(request), 0);
    
    char buffer[TEST_BUFFER_SIZE] = {0};
    read(sock, buffer, TEST_BUFFER_SIZE - 1);
    
    std::string response(buffer);
    assert(response.find("Hello, Test!") != std::string::npos);
    
    close(sock);
}

int main() {
    std::cout << "Starting HTTP server tests...\n";
    
    // 测试1: 基本功能测试
    start_test_server();
    test_http_client();
    std::cout << "Test 1 passed: Basic functionality\n";
    
    // 可以添加更多测试用例...
    
    std::cout << "All tests passed!\n";
    return 0;
}