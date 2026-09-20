#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

// DS18B20 水温传感器类
class TemperatureSensor {
private:
    std::string devicePath;
public:
    // deviceId 格式如: "28-00000xxxxxxx"
    explicit TemperatureSensor(const std::string& deviceId) {
        // Linux 内核驱动（w1-gpio）会将单总线设备映射为虚拟文件
        devicePath = "/sys/bus/w1/devices/" + deviceId + "/w1_slave";
    }

    // 读取当前水温 (摄氏度)
    double readTemperature() {
        std::ifstream file(devicePath);
        if (!file.is_open()) {
            // 如果未连接真实硬件，提供模拟测试温度
            return 25.5; 
        }

        std::string line;
        double temp = 0.0;
        // 逐行解析驱动文件输出（格式中包含 t=25500 形式的千倍摄氏度数值）
        while (std::getline(file, line)) {
            size_t pos = line.find("t=");
            if (pos != std::string::npos) {
                std::string tempStr = line.substr(pos + 2);
                temp = std::stod(tempStr) / 1000.0; // 转换为摄氏度
            }
        }
        return temp;
    }
};

// GPIO 继电器/电机/灯光控制类
class ActuatorGPIO {
private:
    int gpioPin;
    bool state;
public:
    explicit ActuatorGPIO(int pin) : gpioPin(pin), state(false) {}

    void turnOn() {
        state = true;
        std::cout << "[GPIO " << gpioPin << "] 设备已开启。" << std::endl;
    }

    void turnOff() {
        state = false;
        std::cout << "[GPIO " << gpioPin << "] 设备已关闭。" << std::endl;
    }

    bool getState() const { 
        return state; 
    }
};
