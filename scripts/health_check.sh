#!/bin/bash

# 检查HTTP服务器是否响应
response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)

if [ "$response" -eq 200 ]; then
    exit 0
else
    exit 1
fi