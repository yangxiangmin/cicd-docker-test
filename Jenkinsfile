pipeline {
    agent any
    
    // 全局配置：确保快速失败
    options {
        timeout(time: 30, unit: 'MINUTES')  // 设置超时
        retry(0)                           // 禁止自动重试
        skipStagesAfterUnstable()          // 失败后跳过后续阶段
        disableConcurrentBuilds()          // 禁止并发构建
    }
    
    environment {
        // 源代码配置（无需认证）
        REPO_URL = 'https://github.com/yangxiangmin/cicd-docker-test.git'
        BRANCH = 'main'
        
        // 容器镜像配置
        BUILD_IMAGE = 'dockhub.ghtchina.com:6060/ims-cloud/base/base_arm:1.0'
        APP_IMAGE = 'dockhub.ghtchina.com:6060/ims-cloud/http-server:${BUILD_NUMBER}'
        APP_IMAGE_NO_BUILD_NUMBER = 'dockhub.ghtchina.com:6060/ims-cloud/http-server'
        
        // 构建配置
        BUILD_DIR = 'build'
    }
    
    stages {
        stage('Debug Build Number') {
            steps {
                script {
                    echo "当前构建号: ${env.BUILD_NUMBER}"
                    echo "宿主机工作目录env.WORKSPACE: ${env.WORKSPACE}"
                }
            }
        }

        // 阶段1: 拉取源代码 (需要认证)
        stage('Checkout') {
            steps {
                script {
                    try {
                        checkout([
                            $class: 'GitSCM',
                            branches: [[name: env.BRANCH]],
                            userRemoteConfigs: [[url: env.REPO_URL]]
                        ])
                        echo "✅ 源代码检出成功！"
                    } catch (Exception e) {
                        error("❌ 代码源检出失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段2: 拉取编译环境镜像（使用 docker）
        stage('Pull Build Image') {
            steps {
                script {
                    try {
                        sh "docker pull --quiet ${env.BUILD_IMAGE} || docker pull ${env.BUILD_IMAGE} >/dev/null 2>&1"    // 关闭后台输出信息，使用 --quiet 静默参数或者重定向方案

                        // 如果不支持--quiet 静默参数，采用重定向方案
                        // sh "docker pull ${env.BUILD_IMAGE} >/dev/null 2>&1"

                        echo "✅ 编译环境镜像拉取及安装成功！"
                    } catch (Exception e) {
                        error("❌ 编译环境镜像拉取及安装失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段3: 容器化编译
        stage('Build') {
            steps {
                script {
                    try {
                        sh """
                            docker run --rm \
                                -v ${env.WORKSPACE}:/workspace \
                                -w /workspace \
                                ${env.BUILD_IMAGE} \
                                /bin/sh -c '
                                    if ! (yum install -y cmake make gcc-c++); then
                                        echo "=== 安装编译工具 ==="
                                        echo "❌ 错误：无法安装 cmake/make/g++，请检查基础镜像是否支持包管理"
                                        exit 1
                                    fi
                                    echo "=== 开始编译 ==="
                                    mkdir -p ${env.BUILD_DIR} && cd ${env.BUILD_DIR}
                                    cmake .. -DCMAKE_BUILD_TYPE=Release || {
                                        echo "❌ CMake 配置失败";
                                        exit 1;
                                    }
                                    make -j\$(nproc) || {
                                        echo "❌ 编译失败";
                                        exit 1;
                                    }
                                    # 验证测试程序存在
                                    [ -f "test_http_server" ] || {
                                        echo "❌ 错误：测试程序未生成";
                                        exit 1;
                                    }
                                    echo "=== 编译完成 ==="
                                '
                        """
                        echo "✅ 源代码容器化编译成功！"
                    } catch (Exception e) {
                        error("❌ 源代码容器化编译失败: ${e.getMessage()}")
                    }
                }
            }
        }

        //junit 'build/Testing/**/*.xml'    yxmflag
        // 阶段4: 容器化测试（使用 docker）
        // docker run --rm --network=host \  // 添加网络模式，如果容器需要访问宿主机端口，建议使用 --network=host 参数
/*
        stage('Test') {
            steps {
                script {
                    try {
                        sh """
                            docker run --rm --network=host\
                                -v ${env.WORKSPACE}:/workspace \
                                -w /workspace \
                                ${env.BUILD_IMAGE} \
                                /bin/sh -c '
                                    # 安装 CTest 和网络工具
                                    if ! (yum install -y cmake net-tools); then
                                        echo "=== 安装CTest 和网络工具 ==="
                                        echo "❌ 错误：无法安装 cmake/net-tools，请检查基础镜像是否支持包管理"
                                        exit 1
                                    fi
                                    # 检查测试可执行文件
                                    echo "=== 验证构建产物 ==="
                                    ls -l ${env.BUILD_DIR}
                                    [ -f "${env.BUILD_DIR}/test_http_server" ] || {
                                        echo "❌ 错误：测试程序未生成";
                                        exit 1;
                                    }

                                    # 启动服务并检测端口
                                    echo "=== 启动测试服务 ==="
                                    cd ${env.BUILD_DIR}
                                    ./test_http_server &
                                    SERVER_PID=\$!

                                    # 检测服务端口（假设服务监听 8088）
                                    timeout=30
                                    while ! netstat -tuln | grep -q ':8088'; do
                                        sleep 1
                                        timeout=\$((timeout-1))
                                        [ \$timeout -le 0 ] && {
                                            echo "❌ 错误：服务启动超时";
                                            kill \$SERVER_PID 2>/dev/null;
                                            exit 1;
                                        }
                                    done

                                    # 执行测试
                                    echo "=== 运行 CTest ==="
                                    ctest --output-on-failure || {
                                        echo "❌ 测试失败";
                                        kill \$SERVER_PID;
                                        exit 1;
                                    }

                                    # 清理
                                    kill \$SERVER_PID
                                    echo "=== 测试完成 ==="
                                '
                        """
                        //junit *********.xml   yxmflag
                        echo "✅ 源代码容器化测试成功！"
                    } catch (Exception e) {
                        error("❌ 源代码容器化测试失败: ${e.getMessage()}")
                    }
                }
            }
        }
*/
        // 阶段5： 认证阶段
        stage('Login to Registry') {
            steps {
                script {
                    try {
                        // 更安全的做法：使用 Jenkins Credentials ID
                        withCredentials([usernamePassword(
                            credentialsId: 'docker-registry-creds',
                            usernameVariable: 'REGISTRY_USER',
                            passwordVariable: 'REGISTRY_PASS'
                        )]) {
                            sh """
                                docker login -u $REGISTRY_USER -p $REGISTRY_PASS dockhub.ghtchina.com:6060
                            """
                        }
                        echo "✅ 应用镜像仓库认证成功"
                    } catch (Exception e) {
                        error("❌ 应用镜像仓库认证失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段6: 构建应用镜像
        stage('Build Image') {
            steps {
                script {
                    try {
                        // 通过环境变量传递构建号到 Dockerfile
                        withEnv(["BUILD_NUMBER=${env.BUILD_NUMBER}"]) {
                            sh """
                                # 确认 Dockerfile 存在
                                ls -l Dockerfile

                                # 明确指定 Dockerfile
                                docker build -f Dockerfile -t ${env.APP_IMAGE} .
                            """
                        }
                        echo "✅ 应用镜像构建成功！镜像标签: ${env.APP_IMAGE}"
                    } catch (Exception e) {
                        // 失败时明确提示错误类型
                        error("❌ 应用镜像构建失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段7: 推送镜像（使用 docker）
        stage('Push Image') {
            steps {
                script {
                    try {
                        sh """
                            docker push ${env.APP_IMAGE}
                            docker tag ${env.APP_IMAGE} ${env.APP_IMAGE_NO_BUILD_NUMBER}:latest
                            docker push ${env.APP_IMAGE_NO_BUILD_NUMBER}:latest
                        """
                        echo "✅ 应用镜像推送至应用镜像仓库成功！"
                    } catch (Exception e) {
                        error("❌ 应用镜像推送至应用镜像仓库失败: ${e.getMessage()}")
                    }
                }
            }
        }
        
        // 阶段8: 部署
        stage('Deploy') {
            // 指定仅在 agent-02-Deploy（目的部署节点） 节点运行
            agent {
                label 'agent-02-Deploy'
            }

            steps {
                script {
                    try {
                        // 检查资源是否存在
                        def isDeployed = sh(
                            script: 'kubectl get statefulset http-server --ignore-not-found --no-headers',
                            returnStatus: true
                        ) == 0

                        // 存在则删除旧部署
                        if (isDeployed) {
                            echo "🔍 检测到已存在的 http-server 部署，正在卸载..."
                            sh """
                                kubectl delete -f deployment.yaml --ignore-not-found
                                kubectl wait --for=delete statefulset/http-server --timeout=120s || true
                            """
                        }

                        // 部署新版本
                        echo "🚀 开始部署应用镜像..."
                        sh """
                            kubectl apply -f deployment.yaml
                            kubectl rollout status statefulset/http-server --timeout=300s
                        """
                        echo "✅ 应用镜像部署成功！"
                    } catch (Exception e) {
                        error("❌ 应用镜像部署失败: ${e.getMessage()}")
                    }
                }
            }
        }
    }

    // 后置处理
    post {
        always {
            cleanWs()
        }

        failure {
            echo "❌ 流水线执行失败，任务名：${env.JOB_NAME} - 任务构建号：Build ${env.BUILD_NUMBER} - 任务构建地址：${env.BUILD_URL} - 代码仓库地址：${env.REPO_URL}"
        }
        success {
            echo "✅ 流水线执行成功，任务名：${env.JOB_NAME} - 任务构建号：Build ${env.BUILD_NUMBER} - 任务构建地址：${env.BUILD_URL} - 代码仓库地址：${env.REPO_URL}"
        }
    }
}