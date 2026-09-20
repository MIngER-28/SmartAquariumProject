#pragma once
#include "SmartAquarium.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class SocketServer {
private:
    int serverFd;
    int port;
    std::atomic<bool> isRunning;
    std::thread listenThread;
    SmartAquarium& aquarium;// 持有主系统的引用，直接进行业务调用

    // 处理连接进来的客户端
    void handleClient(int clientFd, std::string clientIp) {
        std::cout << "[网络服务] 客户端已连接: " << clientIp << std::endl;
        
        std::string welcome = "=== 智能鱼缸 TCP 控制终端 ===\r\n"
                              "指令格式:\r\n"
                              "  TEMP       - 获取水温\r\n"
                              "  FEED       - 远程喂食\r\n"
                              "  LIGHT ON   - 开灯\r\n"
                              "  LIGHT OFF  - 关灯\r\n"
                              "===============================\r\n";
        send(clientFd, welcome.c_str(), welcome.length(), 0);

        char buffer[1024];
        while (isRunning) {
            memset(buffer, 0, sizeof(buffer));
            // 阻塞接收客户端发来的数据
            ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesRead <= 0) {
                break; // 客户端断开连接
            }

            std::string command(buffer);
            // 清理末尾换行字符
            // 过滤网络命令末尾带有空格、\r、\n 等字符
            command.erase(command.find_last_not_of("\r\n") + 1);

            std::cout << "[网络命令 " << clientIp << "]: " << command << std::endl;

            // 业务指令路由解析
            std::string response;
            if (command == "FEED") {
                aquarium.feed();
                response = "OK: Feeding completed.\r\n";
            } else if (command == "LIGHT ON") {
                aquarium.setLight(true);
                response = "OK: Light ON.\r\n";
            } else if (command == "LIGHT OFF") {
                aquarium.setLight(false);
                response = "OK: Light OFF.\r\n";
            } else if (command == "TEMP") {
                double temp = aquarium.getTemperature();
                response = "TEMP: " + std::to_string(temp) + " C\r\n";
            } else {
                response = "ERROR: Unknown Command!\r\n";
            }

            // 将处理结果回传给客户端
            send(clientFd, response.c_str(), response.length(), 0);
        }

        close(clientFd);// 释放客户端 Socket 文件描述符
        std::cout << "[网络服务] 客户端断开: " << clientIp << std::endl;
    }

    // 服务端监听循环
    vvoid listenLoop() {
        while (isRunning) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            
            // 阻塞等待新客户端连接，成功后返回新的套接字句柄 clientFd
            int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientFd < 0) {
                if (!isRunning) break;
                continue;
            }

            // 解析客户端 IP 地址
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);

            // 线程模型：每来一个客户端连接，创建一个独立线程处理，并调用 detach 分离线程
            std::thread clientThread(&SocketServer::handleClient, this, clientFd, std::string(clientIp));
            clientThread.detach();
        }
    }

public:
    SocketServer(int listenPort, SmartAquarium& aquaRef) 
        : port(listenPort), serverFd(-1), isRunning(false), aquarium(aquaRef) {}

    ~SocketServer() {
        stop();
    }

    bool start() {
        // 1. 创建 TCP 套接字 (AF_INET 为 IPv4，SOCK_STREAM 为 TCP 协议)
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) {
            return false;
        }

        // 2. 设置 SO_REUSEADDR，解决程序重启时端口处于 TIME_WAIT 导致绑定失败的问题
        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 3. 绑定网络地址与端口号
        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // 绑定本机所有网卡 IP
        serverAddr.sin_port = htons(port);       // 主机字节序转换为网络大端字节序

        if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            close(serverFd);
            return false;
        }

        // 4. 将 Socket 模式设置为被动监听状态
        if (listen(serverFd, 5) < 0) {
            close(serverFd);
            return false;
        }

        isRunning = true;
        // 启动独立监听线程，避免阻塞主程序
        listenThread = std::thread(&SocketServer::listenLoop, this);
        std::cout << "[网络服务] TCP Socket 服务已启动，监听端口: " << port << std::endl;
        return true;
    }

    void stop() {
        if (isRunning) {
            isRunning = false;
            if (serverFd >= 0) {
                shutdown(serverFd, SHUT_RDWR);// 关闭 Socket 读写通道以解除 accept 阻塞
                close(serverFd);
            }
            if (listenThread.joinable()) {
                listenThread.join();
            }
            std::cout << "[网络服务] TCP 服务端已安全停止。" << std::endl;
        }
    }
};
