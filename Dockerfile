# 声明构建时的变量
ARG BUILD_NUMBER
RUN echo "当前宿主工作目录:" && pwd

# 使用ARM架构的基础镜像（基于YUM的发行版，如openEuler）
FROM dockhub.ghtchina.com:6060/ims-cloud/base/base_arm:1.0 AS builder

WORKDIR /app
COPY . .

# 构建阶段：安装编译工具链（使用yum）
RUN yum install -y cmake make gcc-c++ && \
    rm -rf build && \  # 强制删除旧目录
    mkdir build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# 运行时镜像（基于YUM的发行版）
FROM dockhub.ghtchina.com:6060/ims-cloud/base/base_arm:1.0

WORKDIR /app
COPY --from=builder /app/build/http_server /app/
COPY scripts/health_check.sh /app/

# 赋予脚本执行权限
RUN chmod +x /app/health_check.sh

# 使用 BUILD_NUMBER 作为镜像标签或版本信息
LABEL build_number=${BUILD_NUMBER}

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
