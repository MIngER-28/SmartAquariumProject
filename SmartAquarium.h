#pragma once
#include "Hardware.h"
#include <mutex>
#include <atomic>
#include <thread>

class SmartAquarium {
private:
    TemperatureSensor tempSensor;
    ActuatorGPIO heaterPin;  // 加热棒控制
    ActuatorGPIO feederPin;  // 自动喂食电机
    ActuatorGPIO lightPin;   // LED 灯光

    double minTemp;
    double maxTemp;
    
    std::atomic<bool> running; // 原子变量，保证多线程下标志位的读写安全
    std::thread monitorThread; // 后台温度监控线程
    std::mutex ctrlMutex;      // 互斥锁，保护硬件设备不被并发冲突操作

    // 后台传感器监控循环
    void monitorLoop() {
        while (running) {
            double currentTemp = tempSensor.readTemperature();
            std::cout << "\n[实时监控] 当前水温: " << currentTemp << " °C" << std::endl;

            {
                // 使用 RAII 机制自动加锁，作用域结束时自动释放锁
                std::lock_guard<std::mutex> lock(ctrlMutex);
                
                // 自动恒温控制算法（双门限阈值控制，防止水温临界值频繁启停）
                if (currentTemp < minTemp && !heaterPin.getState()) {
                    std::cout << "[警告] 水温低于目标，开启加热棒。" << std::endl;
                    heaterPin.turnOn();
                } else if (currentTemp >= maxTemp && heaterPin.getState()) {
                    std::cout << "[提示] 水温达到标准，关闭加热棒。" << std::endl;
                    heaterPin.turnOff();
                }
            }
            // 线程休眠 3 秒，避免过度占用 CPU 资源
            std::this_thread::sleep_for(std::chrono::seconds(3)); // 采样周期: 3秒
        }
    }

public:
    SmartAquarium(const std::string& sensorId, int heaterGpio, int feederGpio, int lightGpio)
        : tempSensor(sensorId), 
          heaterPin(heaterGpio), 
          feederPin(feederGpio), 
          lightPin(lightGpio),
          minTemp(24.0), 
          maxTemp(28.0), 
          running(false) {}

    ~SmartAquarium() {
        stop(); // 析构时安全停止后台线程，防止野线程
    }

    void start() {
        running = true;
        // 启动后台子线程运行 monitorLoop 函数
        monitorThread = std::thread(&SmartAquarium::monitorLoop, this);
    }

    void stop() {
        if (running) {
            running = false;
            if (monitorThread.joinable()) {
                monitorThread.join(); // 等待监控线程优雅退出
            }
        }
    }

    // 获取当前温度
    double getTemperature() {
        return tempSensor.readTemperature();
    }

    // 执行控制指令（加锁保护并发安全）
    void feed() {
        std::lock_guard<std::mutex> lock(ctrlMutex);
        std::cout << "[控制指令] 启动自动喂食系统..." << std::endl;
        feederPin.turnOn();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // 电机转动1.5秒
        feederPin.turnOff();
        std::cout << "[控制指令] 喂食结束。" << std::endl;
    }

    // 线程安全的灯光控制接口
    void setLight(bool enable) {
        std::lock_guard<std::mutex> lock(ctrlMutex);
        if (enable) {
            std::cout << "[控制指令] 开启灯光。" << std::endl;
            lightPin.turnOn();
        } else {
            std::cout << "[控制指令] 关闭灯光。" << std::endl;
            lightPin.turnOff();
        }
    }
};
