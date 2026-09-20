#include "SmartAquarium.h"
#include "SocketServer.h"
#include <iostream>

void printMenu() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "  智能控制鱼缸系统 (C++ Linux Socket)     " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << " 本地控制菜单:" << std::endl;
    std::cout << " 1. 远程/手动喂食" << std::endl;
    std::cout << " 2. 开启灯光" << std::endl;
    std::cout << " 3. 关闭灯光" << std::endl;
    std::cout << " 0. 退出系统" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << " 请输入操作指令: ";
}

int main() {
    // 1. 初始化鱼缸主控 (传感器ID, 加热引脚, 喂食引脚, 灯光引脚)
    SmartAquarium aquarium("28-000001234567", 18, 23, 24);
    aquarium.start(); // 启动传感器采样线程

    // 2. 启动 Socket 网络服务 (监听 8888 端口)
    SocketServer server(8888, aquarium);
    if (!server.start()) {
        std::cerr << "[警告] 网络模块启动失败，仅保留本地控制模式。" << std::endl;
    }

    // 3. 主界面交互循环
    int choice = -1;
    while (choice != 0) {
        printMenu();
        // 针对非数字非法输入的容错清理机制
        if (!(std::cin >> choice)) {
            std::cin.clear();               // 重置 std::cin 错误标记
            std::cin.ignore(1024, '\n');    // 清空缓冲区中的非法字符
            continue;
        }

        switch (choice) {
            case 1:
                aquarium.feed();
                break;
            case 2:
                aquarium.setLight(true);
                break;
            case 3:
                aquarium.setLight(false);
                break;
            case 0:
                std::cout << "正在关闭系统并释放资源..." << std::endl;
                break;
            default:
                std::cout << "无效指令，请重新输入。" << std::endl;
                break;
        }
    }

    // 优雅停机
    server.stop();
    aquarium.stop();

    return 0;
}
