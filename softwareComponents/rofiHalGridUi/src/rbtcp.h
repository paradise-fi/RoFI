#pragma once

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

class TcpSocket;
class Tcp {
    friend class TcpSocketReal;

public:
    static TcpSocket connect(const std::string& ip, uint16_t port);

private:
    Tcp();

    Tcp(Tcp&) = delete;
    Tcp& operator=(Tcp&) = delete;

    static int openSocket(const std::string& ip, uint16_t port);
};

class TcpSocket;

class TcpSocketReal {
    friend class Tcp;
    friend class TcpSocket;

public:
    ~TcpSocketReal();

private:
    typedef std::function<void(const std::vector<uint8_t>& data)> CbType;

    TcpSocketReal(int fd, const std::string& ip, uint16_t port);
    TcpSocketReal(TcpSocketReal&) = delete;
    TcpSocketReal& operator=(TcpSocketReal&) = delete;

    int send(uint8_t* data, size_t size);
    int onReceive(CbType callback);
    void removeCallback(int handle);

    bool isOpen() const { return m_fd >= 0; }

    void reopenSocketLocked();

    static void readTask(void* selfVoid);

    int m_fd;
    const std::string m_ip;
    const uint16_t m_port;
    int m_cbCounter;
    std::map<int, CbType> m_callbacks;
    std::mutex m_mutex;
    TaskHandle_t m_readTask;
};

class TcpSocket {
    friend class Tcp;

public:
    typedef int CallbackHandle;

    ~TcpSocket();

    bool isOpen() const { return m_socket->isOpen(); }

    int send(uint8_t* data, size_t size) {
        return m_socket->send(data, size);
    }
    int send(const std::string& text) {
        return send((uint8_t*)text.c_str(), text.size());
    }

    CallbackHandle onReceive(std::function<void(const std::vector<uint8_t>& data)> callback) {
        return m_socket->onReceive(callback);
    }
    CallbackHandle onReceive(std::function<void(const std::string& text)> callback) {
        return onReceive([=](const std::vector<uint8_t>& data) {
            callback(std::string((char*)data.data(), data.size()));
        });
    }

    void removeCallback(CallbackHandle handle) {
        return m_socket->removeCallback(handle);
    }

private:
    TcpSocket(TcpSocketReal* socket);
    std::shared_ptr<TcpSocketReal> m_socket;
};
};
