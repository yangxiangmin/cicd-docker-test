# 使用ARM架构的基础镜像（基于YUM的发行版，如openEuler）
FROM dockhub.ghtchina.com:6060/ims-cloud/base/base_arm:1.0 AS builder

WORKDIR /app
COPY . .

# 构建阶段：安装编译工具链（使用yum）
RUN yum install -y cmake make gcc-c++ && \
    mkdir build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# 运行时镜像（基于YUM的发行版）
FROM dockhub.ghtchina.com:6060/ims-cloud/base/base_arm:1.0

WORKDIR /app
COPY --from=builder /app/build/http_server /app/
COPY scripts/health_check.sh /app/

# 安装运行时依赖（使用yum）
RUN yum update -y && \
    yum install -y curl && \
    yum clean all && \
    rm -rf /var/cache/yum

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s \
    CMD ./health_check.sh || exit 1

EXPOSE 8088
CMD ["./http_server"]