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
        //APP_IMAGE = 'dockhub.ghtchina.com:6060/ims-cloud/http-server:${BUILD_NUMBER}'
        APP_IMAGE_NO_BUILD_NUMBER = 'dockhub.ghtchina.com:6060/ims-cloud/http-server'
        
        // 构建配置
        BUILD_DIR = 'build'
    }
    
    stages {
        stage('Output Current Build Number') {
            steps {
                script {
                    echo "当前构建号: ${env.BUILD_NUMBER}"
                    echo "宿主机工作目录env.WORKSPACE: ${env.WORKSPACE}"
                }
            }
        }

        // 阶段1: 拉取源代码 (需要认证)
        stage('Checkout Source Code') {
            steps {
                script {
                    try {
                        checkout([
                            $class: 'GitSCM',
                            branches: [[name: env.BRANCH]],
                            userRemoteConfigs: [[url: env.REPO_URL]]
                        ])
                        echo "✅ 源代码检出成功！"

                        dir('deploy') {
                            sh 'cp ../deployment.yaml ./'
                            stash name: 'deployment-config', includes: 'deployment.yaml'    // 将部署文件暂存在Jenkins Master节点上，用于部署节点获取
                        }
                    } catch (Exception e) {
                        error("❌ 代码源检出失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段2: 拉取编译环境镜像（使用 docker）
        stage('Pull Compilation Image') {
            steps {
                script {
                    try {
                        sh "docker pull --quiet ${env.BUILD_IMAGE} || docker pull ${env.BUILD_IMAGE} >/dev/null 2>&1"    // 关闭后台输出信息，使用 --quiet 静默参数或者重定向方案

                        // 如果不支持--quiet 静默参数，采用重定向方案
                        // sh "docker pull ${env.BUILD_IMAGE} >/dev/null 2>&1"

                        echo "✅ 基础镜像拉取及安装成功！"
                    } catch (Exception e) {
                        error("❌ 基础镜像拉取及安装失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段3: 容器化编译及测试
        stage('Containerized Compilation & Test') {
            steps {
                script {
                    try {
                        sh """
                            docker run --rm --network=host \
                                -v ${env.WORKSPACE}:/workspace \
                                -w /workspace \
                                ${env.BUILD_IMAGE} \
                                /bin/sh -c '
                                    # ----------- 环境初始化 -----------
                                    echo "=== 安装编译和测试依赖 ==="
                                    if ! (yum install -y cmake make gcc-c++ net-tools); then
                                        echo "❌ 错误：依赖安装失败，请检查镜像包管理配置"
                                        exit 1
                                    fi

                                    # ----------- 编译阶段 -----------
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
                                    [ -f "test_http_server" ] || {
                                        echo "❌ 错误：编译产物缺失";
                                        exit 1;
                                    }

                                    # ----------- 测试阶段 -----------
                                    echo "=== 启动测试进程并运行测试 ==="
                                    ./test_http_server &
                                    SERVER_PID=\$!

                                    echo "当前测试进程 PID: \$SERVER_PID"

                                    timeout=30
                                    while ! netstat -tuln | grep -q ':8088'; do
                                        sleep 1
                                        timeout=\$((timeout-1))
                                        [ \$timeout -le 0 ] && {
                                            echo "❌ 服务启动超时";
                                            kill \$SERVER_PID 2>/dev/null;
                                            exit 1;
                                        }
                                    done

                                    echo "=== 执行 CTest 测试 ==="
                                    ctest --output-on-failure || {
                                        echo "❌ 测试失败";
                                        kill \$SERVER_PID 2>/dev/null;
                                        exit 1;
                                    }

                                    # ----------- 清理阶段 -----------
                                    kill \$SERVER_PID 2>/dev/null
                                    echo "=== 容器化编译与测试完成 ==="
                                '
                        """
                        echo "✅ 应用容器化编译与测试成功！"
                    } catch (Exception e) {
                        error("❌ 应用容器化编译与测试失败: ${e.getMessage()}")
                    }
                }
            }
        }

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
        stage('Building Application Image') {
            steps {
                script {
                    try {
                        // 直接使用 Groovy 的字符串插值
                        def appImage = "${env.APP_IMAGE_NO_BUILD_NUMBER}:${env.BUILD_NUMBER}"
                        
                        sh """
                            ls -l Dockerfile
                            docker build -f Dockerfile -t ${appImage} .
                        """
                        echo "✅ 应用镜像构建成功！镜像标签: ${appImage}"
                    } catch (Exception e) {
                        error("❌ 应用镜像构建失败: ${e.getMessage()}")
                    }
                }
            }
        }

        // 阶段7: 推送镜像（使用 docker）
        stage('Push Application Image') {
            steps {
                script {
                    try {
                        sh """
                            def appImage = "${env.APP_IMAGE_NO_BUILD_NUMBER}:${env.BUILD_NUMBER}"
                            docker push ${appImage}

                            docker tag ${appImage} ${env.APP_IMAGE_NO_BUILD_NUMBER}:latest
                            docker push ${env.APP_IMAGE_NO_BUILD_NUMBER}:latest
                        """
                        echo "✅ 应用镜像上传至应用镜像仓库成功！"
                    } catch (Exception e) {
                        error("❌ 应用镜像上传至应用镜像仓库失败: ${e.getMessage()}")
                    }
                }
            }
        }
        
        // 阶段8: 部署
        stage('Deploy Application Image') {
            // 指定仅在 agent-02-Deploy（目的部署节点） 节点运行
            agent {
                label 'agent-02-Deploy'
            }
            options {
                skipDefaultCheckout true  // 禁用自动代码检出
            }
            steps {
                script {
                    try {
                        // 获取deployment.yaml
                        unstash 'deployment-config'

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

/*
    // 在 pipeline 的 post 部分添加邮件通知
    post {
        always {
            cleanWs()
        }

        failure {
            script {
                echo "❌ 流水线执行失败，任务名：${env.JOB_NAME} - 任务构建号：Build ${env.BUILD_NUMBER} - 任务构建地址：${env.BUILD_URL} - 代码仓库地址：${env.REPO_URL}"
            
                // 发送失败邮件通知
                emailext (
                    subject: "❌ 构建失败: ${env.JOB_NAME} #${env.BUILD_NUMBER}",
                    body: """
                        <p>构建失败！</p>
                        <p><b>项目名称:</b> ${env.JOB_NAME}</p>
                        <p><b>构建编号:</b> ${env.BUILD_NUMBER}</p>
                        <p><b>构建状态:</b> <span style="color:red;">失败</span></p>
                        <p><b>构建日志:</b> <a href="${env.BUILD_URL}">查看详情</a></p>
                        <p><b>代码仓库:</b> ${env.REPO_URL}</p>
                        <p><b>失败原因:</b> ${currentBuild.currentResult}</p>
                    """,
                    to: 'yangxiangmin@sina.com',
                    mimeType: 'text/html'
                )
            }
        }
    
        success {
            script {
                echo "✅ 流水线执行成功，任务名：${env.JOB_NAME} - 任务构建号：Build ${env.BUILD_NUMBER} - 任务构建地址：${env.BUILD_URL} - 代码仓库地址：${env.REPO_URL}"
            
                // 发送成功邮件通知
                emailext (
                    subject: "✅ 构建成功: ${env.JOB_NAME} #${env.BUILD_NUMBER}",
                    body: """
                        <p>构建成功完成！</p>
                        <p><b>项目名称:</b> ${env.JOB_NAME}</p>
                        <p><b>构建编号:</b> ${env.BUILD_NUMBER}</p>
                        <p><b>构建状态:</b> <span style="color:green;">成功</span></p>
                        <p><b>构建日志:</b> <a href="${env.BUILD_URL}">查看详情</a></p>
                        <p><b>代码仓库:</b> ${env.REPO_URL}</p>
                        <p><b>生成镜像:</b> ${env.APP_IMAGE_NO_BUILD_NUMBER}:${env.BUILD_NUMBER}</p>
                    """,
                    to: 'yangxiangmin@sina.com',
                    mimeType: 'text/html'
                )
            }
        }
    }
*/

    // 后置处理
    post {
        always {
            cleanWs()
        }

        failure {
            echo "❌ 流水线执行失败，任务名：${env.JOB_NAME} - 任务构建号：Build ${env.BUILD_NUMBER} - 代码仓库地址：${env.REPO_URL} - 任务构建地址：${env.BUILD_URL} - 镜像仓库及名称：${env.APP_IMAGE_NO_BUILD_NUMBER}:${env.BUILD_NUMBER}"
        }
        success {
            echo "✅ 流水线执行成功，任务名：${env.JOB_NAME} - 任务构建号：Build ${env.BUILD_NUMBER} - 代码仓库地址：${env.REPO_URL} - 任务构建地址：${env.BUILD_URL} - 镜像仓库及名称：${env.APP_IMAGE_NO_BUILD_NUMBER}:${env.BUILD_NUMBER}"
        }
    }
}