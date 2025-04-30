cmake_minimum_required(VERSION 3.10)
project(HTTP_Server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 可执行文件
add_executable(http_server src/http_server.cpp)

# 安装目标
install(TARGETS http_server DESTINATION bin)

# 包含测试
enable_testing()
add_subdirectory(tests)