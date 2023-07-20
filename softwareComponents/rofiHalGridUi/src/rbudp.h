#pragma once

#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace rb {

class Udp {
public:
    typedef int CallbackHandle;
    static int send(uint8_t* data, size_t size, uint16_t port, const std::string& ip = "255.255.255.255");
    static int send(const char* str, uint16_t port, const std::string& ip = "255.255.255.255") {
        return send((uint8_t*)str, strlen(str), port, ip);
    }

    static CallbackHandle onReceive(uint16_t port, std::function<void(const std::vector<uint8_t>& data, const std::string& sourceIp)> callback);
    static CallbackHandle onReceive(uint16_t port, std::function<void(const std::string& data, const std::string& sourceIp)> callback) {
        return onReceive(port, [=](const std::vector<uint8_t>& data, const std::string& sourceIp) {
            callback(std::string((char*)data.data(), data.size()), sourceIp);
        });
    };

    static void removeCallback(CallbackHandle handle);

    static int openSocket(uint16_t port);

private:
    struct CallbackInfo {
        std::function<void(const std::vector<uint8_t>& data, const std::string& sourceIp)> callback;
        uint16_t port;
    };

    Udp();
    Udp(Udp&) = delete;
    Udp& operator=(Udp&) = delete;

    static Udp& get();

    static void readTask(void*);
    void reopenSocketLocked(std::map<uint16_t, int>::iterator socket_itr);

    std::map<uint16_t, int> m_open_sockets;
    std::map<CallbackHandle, std::unique_ptr<CallbackInfo>> m_callbacks;
    int m_callback_id_counter;
    TaskHandle_t m_read_task;

    std::mutex m_mutex;
};
};
