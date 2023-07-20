#include "rbudp.h"

#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

static const char* TAG = "RbProtocol";

namespace rb {

Udp::Udp()
    : m_callback_id_counter(0)
    , m_read_task(nullptr) {}

Udp& Udp::get() {
    static Udp instance;
    return instance;
}

int Udp::send(uint8_t* data, size_t size, uint16_t port, const std::string& ip) {
    struct sockaddr_in addr = {
        .sin_len = sizeof(struct sockaddr_in),
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { 0 },
        .sin_zero = { 0 },
    };
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    auto& self = Udp::get();
    std::lock_guard<std::mutex> l(self.m_mutex);

    int socketFd;
    auto socketItr = self.m_open_sockets.find(port);
    const bool ourSocket = socketItr == self.m_open_sockets.end();
    if (ourSocket) {
        socketFd = openSocket(port);
    } else {
        socketFd = socketItr->second;
    }

    if (socketFd == -1) {
        return -1;
    }

    const int res = sendto(socketFd, data, size, 0, (sockaddr*)&addr, sizeof(addr));
    if (ourSocket) {
        close(socketFd);
    }
    return res;
}

Udp::CallbackHandle Udp::onReceive(uint16_t port, std::function<void(const std::vector<uint8_t>& data, const std::string& sourceIp)> callback) {
    auto& self = Udp::get();

    std::lock_guard<std::mutex> l(self.m_mutex);
    if (self.m_open_sockets.find(port) == self.m_open_sockets.end()) {
        int fd = openSocket(port);
        if (fd == -1) {
            return -1;
        }
        self.m_open_sockets[port] = fd;
    }

    std::unique_ptr<CallbackInfo> info(new CallbackInfo);
    info->callback = std::move(callback);
    info->port = port;

    auto handle = ++self.m_callback_id_counter;
    self.m_callbacks[handle] = std::move(info);

    if (self.m_read_task == nullptr) {
        xTaskCreate(Udp::readTask, "RbProtUdpRead", 6182, NULL, 5, &self.m_read_task);
    }

    return handle;
}

void Udp::removeCallback(CallbackHandle handle) {
    auto& self = Udp::get();
    std::lock_guard<std::mutex> l(self.m_mutex);

    auto itr = self.m_callbacks.find(handle);
    if (itr == self.m_callbacks.end())
        return;

    auto info = std::move(itr->second);
    self.m_callbacks.erase(itr);

    for (itr = self.m_callbacks.begin(); itr != self.m_callbacks.end(); ++itr) {
        if (itr->second->port == info->port) {
            return;
        }
    }

    auto socketItr = self.m_open_sockets.find(info->port);
    if (socketItr == self.m_open_sockets.end())
        return;
    self.m_open_sockets.erase(socketItr);
    close(socketItr->second);
}

int Udp::openSocket(uint16_t port) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1) {
        ESP_LOGE(TAG, "failed to create socket: %s", strerror(errno));
        return -1;
    }

    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        ESP_LOGE(TAG, "failed to set SO_REUSEADDR: %s", strerror(errno));
        close(fd);
        return -1;
    }

    struct sockaddr_in addr_bind;
    memset(&addr_bind, 0, sizeof(addr_bind));
    addr_bind.sin_family = AF_INET;
    addr_bind.sin_port = htons(port);
    addr_bind.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr*)&addr_bind, sizeof(addr_bind)) < 0) {
        ESP_LOGE(TAG, "failed to bind socket: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

void Udp::reopenSocketLocked(std::map<uint16_t, int>::iterator socket_itr) {
    close(socket_itr->second);
    while (true) {
        socket_itr->second = openSocket(socket_itr->first);
        if (socket_itr->second != -1) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void Udp::readTask(void*) {
    auto& self = Udp::get();

    struct sockaddr_in addr;
    socklen_t addr_len;
    std::vector<uint8_t> buf;
    buf.resize(64);
    ssize_t res;
    std::string ip;

    while (true) {
        self.m_mutex.lock();
        for (auto itr = self.m_open_sockets.begin(); itr != self.m_open_sockets.end(); ++itr) {
            buf.resize(buf.capacity());
            while (true) {
                res = recvfrom(itr->second, buf.data(), buf.size(), MSG_PEEK | MSG_DONTWAIT, NULL, NULL);
                if (res < 0) {
                    const auto err = errno;
                    if (err != EAGAIN && err != EWOULDBLOCK)
                        ESP_LOGE(TAG, "error in recvfrom: %d %s!", err, strerror(err));
                    if (err == EBADF)
                        self.reopenSocketLocked(itr);
                    break;
                }

                if (res < buf.size() || buf.size() >= 1024) {
                    buf.resize(res);
                    break;
                }
                buf.resize(buf.size() + 16);
            }

            if (res < 0) {
                continue;
            }

            addr_len = sizeof(struct sockaddr_in);
            const auto pop_res = recvfrom(itr->second, buf.data(), 0, 0, (struct sockaddr*)&addr, &addr_len);
            if (pop_res < 0) {
                const auto err = errno;
                ESP_LOGE(TAG, "error in recvfrom: %d %s!", err, strerror(err));
                if (err == EBADF)
                    self.reopenSocketLocked(itr);
                continue;
            }

            ip.resize(INET_ADDRSTRLEN + 1);
            inet_ntop(AF_INET, &addr.sin_addr, (char*)ip.data(), ip.size());

            for (auto cb = self.m_callbacks.begin(); cb != self.m_callbacks.end(); ++cb) {
                if (cb->second->port != itr->first)
                    continue;
                cb->second->callback(buf, ip);
            }
        }

        self.m_mutex.unlock();

        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
};
