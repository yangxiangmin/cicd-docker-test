# 使用ARM架构的基础镜像
FROM dockhub.ghtchina.com:6060/ims-cloud/base/base_arm:1.0 AS builder

WORKDIR /app
COPY . .

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# 运行时镜像
FROM dockhub.ghtchina.com:6060/ims-cloud/base/runtime_arm:1.0

WORKDIR /app
COPY --from=builder /app/build/http_server /app/
COPY scripts/health_check.sh /app/

# 安装依赖
RUN apt-get update && \
    apt-get install -y curl && \
    rm -rf /var/lib/apt/lists/*

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s \
    CMD ./health_check.sh || exit 1

EXPOSE 8080
CMD ["./http_server"]