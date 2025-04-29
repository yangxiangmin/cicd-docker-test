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
        
        // 构建配置
        BUILD_DIR = 'build'
    }
    
    stages {
        // 阶段1: 拉取源代码（无需认证）
        //stage('Checkout') {
        //    steps {
        //        script {
        //            try {
        //                checkout([
        //                   $class: 'GitSCM',
        //                   branches: [[name: env.BRANCH]],
        //                   userRemoteConfigs: [[
        //                        url: env.REPO_URL,
        //                        credentialsId: ''  // 显式声明无需认证
        //                    ]]
        //                ])
        //                echo "✅ 已完成代码检出！"
        //            } catch (Exception e) {
        //                error("❌ 代码检出失败: ${e.getMessage()}")
        //           }
        //        }
        //    }
        //}

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
                        echo "✅ 已完成代码检出！"
                    } catch (Exception e) {
                        error("❌ 代码检出失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段2: 拉取编译环境镜像（使用 nerdctl）
        stage('Pull Build Image') {
            steps {
                script {
                    try {
                        //sh "nerdctl pull ${env.BUILD_IMAGE}"          // 拉取过程有很多后台输出信息

                        sh "nerdctl pull --quiet ${env.BUILD_IMAGE}"    // 关闭输出信息，需要确保nerdctl 版本支持 --quiet 静默参数

                        // 如果不支持--quiet 静默参数，采用重定向方案
                        // sh "nerdctl pull ${env.BUILD_IMAGE} >/dev/null 2>&1"

                        echo "✅ 已完成编译环境镜像拉取！"
                    } catch (Exception e) {
                        error("❌ 编译环境镜像拉取失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段3: 容器化编译（使用 nerdctl）
        stage('Build') {
            steps {
                script {
                    try {
                        sh """
                            nerdctl run --rm \
                                -v ${env.WORKSPACE}:/workspace \
                                -w /workspace \
                                ${env.BUILD_IMAGE} \
                                /bin/sh -c '
                                    echo "=== 安装编译工具 ==="
                                    if ! (yum install -y cmake make gcc-c++ 2>/dev/null); then
                                        echo "❌ 错误：无法安装 cmake/make/g++，请检查基础镜像是否支持包管理"
                                        exit 1
                                    fi
                                    echo "=== 开始编译 ==="
                                    mkdir -p ${env.BUILD_DIR} && cd ${env.BUILD_DIR}
                                    cmake .. -DCMAKE_BUILD_TYPE=Release
                                    make -j\$(nproc)
                                    echo "=== 编译完成 ==="
                                '
                        """
                        echo "✅ 已完成容器化编译！"
                    } catch (Exception e) {
                        error("❌ 容器化编译失败: ${e.getMessage()}")
                    }
                }
            }
        }
        
        // 阶段4: 容器化测试（使用 nerdctl）
        stage('Test') {
            steps {
                script {
                    try {
                        sh """
                            nerdctl run --rm \
                                -v ${env.WORKSPACE}:/workspace \
                                -w /workspace \
                                ${env.BUILD_IMAGE} \
                                /bin/sh -c '
                                    echo "=== 开始测试 ==="
                                    cd ${env.BUILD_DIR}
                                    ./http_server &
                                    SERVER_PID=\$!
                                    sleep 2
                                    ctest --output-on-failure
                                    kill \$SERVER_PID
                                    echo "=== 测试完成 ==="
                                '
                        """
                        junit 'build/Testing/**/*.xml'
                        echo "✅ 已完成容器化测试！"
                    } catch (Exception e) {
                        error("❌ 容器化测试失败: ${e.getMessage()}")
                    }
                }
            }
        }
        
        // 阶段5: 构建应用镜像（使用 nerdctl）
        stage('Build Image') {
            steps {
                script {
                    try {
                        sh """
                            nerdctl build \
                                --build-arg BUILD_NUMBER=${env.BUILD_NUMBER} \
                                -t ${env.APP_IMAGE} .
                        """
                        echo "✅ 已完成应用镜像构建！"
                    } catch (Exception e) {
                        error("❌ 应用镜像构建失败: ${e.getMessage()}")
                    }
                }
            }
        }
        
        // 阶段6: 推送镜像（使用 nerdctl）
        stage('Push Image') {
            steps {
                script {
                    try {
                        sh """
                            nerdctl push ${env.APP_IMAGE}
                            nerdctl tag ${env.APP_IMAGE} ${env.APP_IMAGE}:latest
                            nerdctl push ${env.APP_IMAGE}:latest
                        """
                        echo "✅ 已完成应用镜像推送！"
                    } catch (Exception e) {
                        error("❌ 应用镜像推送失败: ${e.getMessage()}")
                    }
                }
            }
        }
        
        // 阶段7: 部署（仅 main 分支执行）
        stage('Deploy') {
            when {
                branch 'main'
            }
            steps {
                script {
                    try {
                        withKubeConfig([credentialsId: 'k8s-creds']) {
                            sh """
                                kubectl apply -f deployment.yaml
                                kubectl rollout status deployment/http-server --timeout=300s
                            """
                        }
                        echo "✅ 已完成应用镜像部署！"
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
        success {
            slackSend(
                color: 'good',
                message: "✅ HTTP Server 部署成功: ${env.JOB_NAME} #${env.BUILD_NUMBER}"
            )
        }
        failure {
            slackSend(
                color: 'danger',
                message: "❌ HTTP Server 部署失败: ${env.JOB_NAME} #${env.BUILD_NUMBER}"
            )
        }
    }
}