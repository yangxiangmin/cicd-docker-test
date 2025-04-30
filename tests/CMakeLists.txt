# 添加测试可执行文件
add_executable(test_http_server test_http_server.cpp)

# 链接主程序库（假设主程序库名为 http_server_lib）
# target_link_libraries(test_http_server PRIVATE http_server_lib)

# 设置输出目录为 build（与主程序一致）
set_target_properties(test_http_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 添加测试用例
add_test(
    NAME http_server_functional_test
    COMMAND test_http_server
)