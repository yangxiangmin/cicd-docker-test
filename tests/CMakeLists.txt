# 测试可执行文件
add_executable(test_http_server test_http_server.cpp)
target_link_libraries(test_http_server)

# 添加测试
add_test(
    NAME http_server_functional_test
    COMMAND test_http_server
)