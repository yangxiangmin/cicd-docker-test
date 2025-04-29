pipeline {
    agent any
    
    environment {
        // 源代码配置
        REPO_URL = 'https://github.com/yangxiangmin/cicd-docker-test.git'
        BRANCH = 'main'
        
        // Docker配置
        BUILD_IMAGE = 'dockhub.ghtchina.com:6060/ims-cloud/base/base_arm:1.0'
        APP_IMAGE = 'dockhub.ghtchina.com:6060/ims-cloud/http-server:${BUILD_NUMBER}'
        DOCKER_CREDENTIALS = 'docker-registry-creds'
        
        // 构建配置
        BUILD_DIR = 'build'
    }
    
    stages {
        // 阶段1: 拉取源代码
        stage('Checkout') {
            steps {
                checkout([
                    $class: 'GitSCM',
                    branches: [[name: env.BRANCH]],
                    userRemoteConfigs: [[url: env.REPO_URL]]
                ])
                echo "✅ 已完成代码检出！"
            }
        }
        
        // 阶段2: 拉取编译环境镜像
        stage('Pull Build Image') {
            steps {
                script {
                    docker.withRegistry('https://dockhub.ghtchina.com:6060', env.DOCKER_CREDENTIALS) {
                        docker.image(env.BUILD_IMAGE).pull()
                    }
                }
                echo "✅ 已完成编译环境镜像拉取！"
            }
        }
        
        // 阶段3: 容器化编译
        stage('Build') {
            steps {
                script {
                    docker.image(env.BUILD_IMAGE).inside("-v ${env.WORKSPACE}:/workspace -w /workspace") {
                        sh '''
                            echo "=== 开始编译 ==="
                            mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR}
                            cmake .. -DCMAKE_BUILD_TYPE=Release
                            make -j$(nproc)
                            echo "=== 编译完成 ==="
                        '''
                    }
                }
                echo "✅ 已完成容器化编译！"
            }
        }
        
        // 阶段4: 容器化测试
        stage('Test') {
            steps {
                script {
                    docker.image(env.BUILD_IMAGE).inside("-v ${env.WORKSPACE}:/workspace -w /workspace") {
                        sh '''
                            echo "=== 开始测试 ==="
                            cd ${BUILD_DIR}
                            # 启动服务器后台运行
                            ./http_server &
                            SERVER_PID=$!
                            sleep 2 # 等待服务器启动
                            
                            # 运行测试
                            ctest --output-on-failure
                            
                            # 停止服务器
                            kill $SERVER_PID
                            echo "=== 测试完成 ==="
                        '''
                    }
                    junit 'build/Testing/**/*.xml'  // 收集测试报告
                }
                echo "✅ 已完成容器化测试！"
            }
        }
        
        // 阶段5: 构建应用镜像
        stage('Build Image') {
            steps {
                script {
                    docker.withRegistry('https://dockhub.ghtchina.com:6060', env.DOCKER_CREDENTIALS) {
                        def appImage = docker.build(env.APP_IMAGE, "--build-arg BUILD_NUMBER=${env.BUILD_NUMBER} .")
                    }
                }
                echo "✅ 已完成应用镜像构建！"
            }
        }
        
        // 阶段6: 推送镜像
        stage('Push Image') {
            steps {
                script {
                    docker.withRegistry('https://dockhub.ghtchina.com:6060', env.DOCKER_CREDENTIALS) {
                        docker.image(env.APP_IMAGE).push()
                        docker.image(env.APP_IMAGE).push('latest')
                    }
                }
                echo "✅ 已完成应用镜像推送！"
            }
        }
        
        // 阶段7: 部署
        stage('Deploy') {
            when {
                branch 'main'
            }
            steps {
                script {
                    withKubeConfig([credentialsId: 'k8s-creds']) {
                        sh """
                            kubectl apply -f deployment.yaml
                            kubectl rollout status deployment/http-server --timeout=300s
                        """
                    }
                }
                echo "✅ 已完成应用镜像部署！"
            }
        }
    }
    
    post {
        always {
            cleanWs()
        }
        success {
            slackSend(color: 'good', message: "HTTP Server部署成功: ${env.JOB_NAME} #${env.BUILD_NUMBER}")
        }
        failure {
            slackSend(color: 'danger', message: "HTTP Server部署失败: ${env.JOB_NAME} #${env.BUILD_NUMBER}")
        }
    }
}