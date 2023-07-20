#include "rbtcp.h"

#include "esp_log.h"
#include <cstring>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

static const char* TAG = "RbProtocol";

namespace rb {

int Tcp::openSocket(const std::string& ip, uint16_t port) {
    struct sockaddr_in addr = {
        .sin_len = sizeof(struct sockaddr_in),
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { 0 },
        .sin_zero = { 0 },
    };
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        auto err = errno;
        ESP_LOGE(TAG, "error in socket: %d %s!", err, strerror(err));
        return -1;
    }

    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        auto err = errno;
        ESP_LOGE(TAG, "error in connect: %d %s!", err, strerror(err));
        close(fd);
        return -1;
    }
    return fd;
}

TcpSocket Tcp::connect(const std::string& ip, uint16_t port) {
    return TcpSocket(new TcpSocketReal(openSocket(ip, port), ip, port));
}

TcpSocketReal::TcpSocketReal(int fd, const std::string& ip, uint16_t port)
    : m_fd(fd)
    , m_ip(ip)
    , m_port(port)
    , m_cbCounter(0) {
    if (fd >= 0)
        xTaskCreate(readTask, "RbTcpRead", 4096, this, 5, &m_readTask);
}

TcpSocketReal::~TcpSocketReal() {
    m_mutex.lock();
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    m_mutex.unlock();

    while (eTaskGetState(m_readTask) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int TcpSocketReal::send(uint8_t* data, size_t size) {
    return ::send(m_fd, data, size, 0);
}

int TcpSocketReal::onReceive(CbType callback) {
    std::lock_guard<std::mutex> l(m_mutex);

    int handle = ++m_cbCounter;
    m_callbacks[handle] = std::move(callback);
    return handle;
}

void TcpSocketReal::removeCallback(int handle) {
    std::lock_guard<std::mutex> l(m_mutex);
    auto itr = m_callbacks.find(handle);
    if (itr != m_callbacks.end()) {
        m_callbacks.erase(itr);
    }
}

void TcpSocketReal::reopenSocketLocked() {
    close(m_fd);
    while (true) {
        m_fd = Tcp::openSocket(m_ip, m_port);
        if (m_fd >= 0)
            break;
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void TcpSocketReal::readTask(void* selfVoid) {
    auto* self = (TcpSocketReal*)selfVoid;

    {
        std::vector<uint8_t> buf;
        buf.resize(64);
        ssize_t res;

        self->m_mutex.lock();
        while (self->m_fd >= 0) {
            self->m_mutex.unlock();
            vTaskDelay(pdMS_TO_TICKS(25));
            self->m_mutex.lock();

            buf.resize(buf.capacity());
            size_t bufOff = 0;
            while (true) {
                const auto restSize = buf.size() - bufOff;
                res = recv(self->m_fd, buf.data() + bufOff, restSize, MSG_DONTWAIT);
                if (res < 0) {
                    const auto err = errno;
                    if (err != EAGAIN && err != EWOULDBLOCK)
                        ESP_LOGE(TAG, "error in recvfrom: %d %s!", err, strerror(err));
                    if (err == EBADF)
                        self->reopenSocketLocked();
                    break;
                }

                if (res < restSize || buf.size() >= 1024) {
                    buf.resize(bufOff + res);
                    break;
                }
                bufOff += res;
                buf.resize(buf.size() + 16);
            }

            if (res < 0) {
                continue;
            }

            for (auto cb = self->m_callbacks.begin(); cb != self->m_callbacks.end(); ++cb) {
                cb->second(buf);
            }
        }
    }
    vTaskDelete(NULL);
}

TcpSocket::TcpSocket(TcpSocketReal* socket)
    : m_socket(socket) {
}

TcpSocket::~TcpSocket() {}
};
