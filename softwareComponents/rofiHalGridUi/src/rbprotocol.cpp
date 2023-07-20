#include <errno.h>
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "rbjson.h"
#include "rbprotocol.h"

using namespace rbjson;

static const char* TAG = "RbProtocol";

#define MS_TO_TICKS(ms) ((portTICK_PERIOD_MS <= ms) ? (ms / portTICK_PERIOD_MS) : 1)

#define MUST_ARRIVE_TIMER_PERIOD MS_TO_TICKS(100)
#define MUST_ARRIVE_ATTEMPTS 15

namespace rb {

Protocol::Protocol(const char* owner, const char* name, const char* description, Protocol::callback_t callback) {
    m_owner = owner;
    m_name = name;
    m_desc = description;
    m_callback = callback;

    m_task_send = nullptr;
    m_task_recv = nullptr;

    m_socket = -1;

    m_sendQueue = xQueueCreate(32, sizeof(QueueItem));

    m_read_counter = 0;
    m_write_counter = 0;

    m_mustarrive_e = 0;
    m_mustarrive_f = 0xFFFFFFFF;

    memset(&m_possessed_addr, 0, sizeof(SockAddr));
}

Protocol::~Protocol() {
    stop();
}

void Protocol::start(uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_task_send != nullptr) {
        return;
    }

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == -1) {
        ESP_LOGE(TAG, "failed to create socket: %s", strerror(errno));
        return;
    }

    int enable = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        ESP_LOGE(TAG, "failed to set SO_REUSEADDR: %s", strerror(errno));
        close(m_socket);
        m_socket = -1;
        return;
    }

    struct sockaddr_in addr_bind;
    memset(&addr_bind, 0, sizeof(addr_bind));
    addr_bind.sin_family = AF_INET;
    addr_bind.sin_port = htons(port);
    addr_bind.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_socket, (struct sockaddr*)&addr_bind, sizeof(addr_bind)) < 0) {
        ESP_LOGE(TAG, "failed to bind socket: %s", strerror(errno));
        close(m_socket);
        m_socket = -1;
        return;
    }

    xTaskCreate(&Protocol::send_task_trampoline, "rbctrl_send", 2560, this, 9, &m_task_send);
    xTaskCreate(&Protocol::recv_task_trampoline, "rbctrl_recv", 4096, this, 10, &m_task_recv);
}

void Protocol::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_task_send == nullptr) {
        return;
    }

    QueueItem it = { };
    xQueueSend(m_sendQueue, &it, portMAX_DELAY);

    if (m_socket != -1) {
        close(m_socket);
        m_socket = -1;
    }

    m_task_send = nullptr;
    m_task_recv = nullptr;
}

bool Protocol::is_possessed() const {
    m_mutex.lock();
    bool res = m_possessed_addr.port != 0;
    m_mutex.unlock();
    return res;
}

bool Protocol::get_possessed_addr(Protocol::SockAddr& addr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_possessed_addr.port == 0)
        return false;
    addr = m_possessed_addr;
    return true;
}

bool Protocol::is_mustarrive_complete(uint32_t id) const {
    if (id == UINT32_MAX)
        return true;
    std::lock_guard<std::mutex> l(m_mustarrive_mutex);
    for (const auto& it : m_mustarrive_queue) {
        if (it.id == id)
            return false;
    }
    return true;
}

uint32_t Protocol::send_mustarrive(const char* cmd, Object* params) {
    SockAddr addr;
    if (!get_possessed_addr(addr)) {
        ESP_LOGW(TAG, "can't send, the device was not possessed yet.");
        delete params;
        return UINT32_MAX;
    }

    if (params == NULL) {
        params = new Object();
    }

    params->set("c", cmd);

    MustArrive mr;
    mr.pkt = params;
    mr.attempts = 0;

    m_mustarrive_mutex.lock();
    const uint32_t id = m_mustarrive_e++;
    mr.id = id;
    params->set("e", mr.id);
    m_mustarrive_queue.emplace_back(mr);
    send(addr, params);
    m_mustarrive_mutex.unlock();

    return id;
}

void Protocol::send(const char* cmd, Object* obj) {
    SockAddr addr;
    if (!get_possessed_addr(addr)) {
        ESP_LOGW(TAG, "can't send, the device was not possessed yet.");
        return;
    }
    send(addr, cmd, obj);
}

void Protocol::send(const SockAddr& addr, const char* cmd, Object* obj) {
    std::unique_ptr<Object> autoptr;
    if (obj == NULL) {
        obj = new Object();
        autoptr.reset(obj);
    }

    obj->set("c", new String(cmd));
    send(addr, obj);
}

void Protocol::send(const SockAddr& addr, Object* obj) {
    m_mutex.lock();
    const int n = m_write_counter++;
    m_mutex.unlock();

    obj->set("n", new Number(n));
    const auto str = obj->str();
    send(addr, str.c_str(), str.size());
}

void Protocol::send(const SockAddr& addr, const char* buf) {
    send(addr, buf, strlen(buf));
}

void Protocol::send(const SockAddr& addr, const char* buf, size_t size) {
    if (size == 0)
        return;

    QueueItem it;
    it.addr = addr;
    it.buf = new char[size];
    it.size = size;
    memcpy(it.buf, buf, size);

    if (xQueueSend(m_sendQueue, &it, pdMS_TO_TICKS(200)) != pdTRUE) {
        ESP_LOGE(TAG, "failed to send - queue full!");
        delete[] it.buf;
    }
}

void Protocol::send_log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    send_log(fmt, args);
    va_end(args);
}

void Protocol::send_log(const char* fmt, va_list args) {
    char static_buf[256];
    std::unique_ptr<char[]> dyn_buf;
    char* used_buf = static_buf;

    int fmt_len = vsnprintf(static_buf, sizeof(static_buf), fmt, args);
    if (fmt_len >= sizeof(static_buf)) {
        dyn_buf.reset(new char[fmt_len + 1]);
        used_buf = dyn_buf.get();
        vsnprintf(dyn_buf.get(), fmt_len + 1, fmt, args);
    }

    send_log(std::string(used_buf));
}

void Protocol::send_log(const std::string& str) {
    Object* pkt = new Object();
    pkt->set("msg", str);
    send_mustarrive("log", pkt);
}

void Protocol::send_task_trampoline(void* ctrl) {
    ((Protocol*)ctrl)->send_task();
}

void Protocol::send_task() {
    TickType_t mustarrive_next;
    QueueItem it;
    struct sockaddr_in send_addr = {
        .sin_len = sizeof(struct sockaddr_in),
        .sin_family = AF_INET,
        .sin_port = 0,
        .sin_addr = { 0 },
        .sin_zero = { 0 },
    };

    m_mutex.lock();
    const int socket_fd = m_socket;
    m_mutex.unlock();

    mustarrive_next = xTaskGetTickCount() + MUST_ARRIVE_TIMER_PERIOD;

    while (true) {
        for (size_t i = 0; xQueueReceive(m_sendQueue, &it, MS_TO_TICKS(10)) == pdTRUE && i < 16; ++i) {
            if (it.buf == nullptr) {
                goto exit;
            }

            send_addr.sin_port = it.addr.port;
            send_addr.sin_addr = it.addr.ip;

            int res = ::sendto(socket_fd, it.buf, it.size, 0, (struct sockaddr*)&send_addr, sizeof(struct sockaddr_in));
            if (res < 0) {
                ESP_LOGE(TAG, "error in sendto: %d %s!", errno, strerror(errno));
            }
            delete[] it.buf;
        }

        if (xTaskGetTickCount() >= mustarrive_next) {
            m_mustarrive_mutex.lock();
            if (m_mustarrive_queue.size() != 0) {
                resend_mustarrive_locked();
            }
            m_mustarrive_mutex.unlock();
            mustarrive_next = xTaskGetTickCount() + MUST_ARRIVE_TIMER_PERIOD;
        }
    }

exit:
    vTaskDelete(nullptr);
}

void Protocol::resend_mustarrive_locked() {
    bool possesed;
    struct sockaddr_in send_addr = {
        .sin_len = sizeof(struct sockaddr_in),
        .sin_family = AF_INET,
        .sin_port = 0,
        .sin_addr = { 0 },
        .sin_zero = { 0 },
    };
    {
        SockAddr addr;
        possesed = get_possessed_addr(addr);
        send_addr.sin_port = addr.port;
        send_addr.sin_addr = addr.ip;
    }

    for (auto itr = m_mustarrive_queue.begin(); itr != m_mustarrive_queue.end();) {
        if (possesed) {
            m_mutex.lock();
            const int n = m_write_counter++;
            m_mutex.unlock();

            (*itr).pkt->set("n", n);

            const auto str = (*itr).pkt->str();

            int res = ::sendto(m_socket, str.c_str(), str.size(), 0, (struct sockaddr*)&send_addr, sizeof(struct sockaddr_in));
            if (res < 0) {
                ESP_LOGE(TAG, "error in sendto: %d %s!", errno, strerror(errno));
            }
        }

        if ((*itr).attempts != -1 && ++(*itr).attempts >= MUST_ARRIVE_ATTEMPTS) {
            delete (*itr).pkt;
            itr = m_mustarrive_queue.erase(itr);
        } else {
            ++itr;
        }
    }
}

void Protocol::recv_task_trampoline(void* ctrl) {
    ((Protocol*)ctrl)->recv_task();
}

void Protocol::recv_task() {
    m_mutex.lock();
    const int socket_fd = m_socket;
    m_mutex.unlock();

    struct sockaddr_in addr;
    socklen_t addr_len;
    size_t buf_size = 64;
    char* buf = (char*)malloc(buf_size);
    ssize_t res;

    while (true) {
        while (true) {
            res = recvfrom(socket_fd, buf, buf_size, MSG_PEEK, NULL, NULL);
            if (res < 0) {
                const auto err = errno;
                ESP_LOGE(TAG, "error in recvfrom: %d %s!", err, strerror(err));
                if (err == EBADF)
                    goto exit;
                vTaskDelay(MS_TO_TICKS(10));
                continue;
            }

            if (res < buf_size)
                break;
            buf_size += 16;
            buf = (char*)realloc(buf, buf_size);
        }

        addr_len = sizeof(struct sockaddr_in);
        const auto pop_res = recvfrom(socket_fd, buf, 0, 0, (struct sockaddr*)&addr, &addr_len);
        if (pop_res < 0) {
            const auto err = errno;
            ESP_LOGE(TAG, "error in recvfrom: %d %s!", err, strerror(err));
            if (err == EBADF)
                goto exit;
            vTaskDelay(MS_TO_TICKS(10));
            continue;
        }

        {
            std::unique_ptr<Object> pkt(parse(buf, res));
            if (!pkt) {
                ESP_LOGE(TAG, "failed to parse the packet's json");
            } else {
                SockAddr sa = {
                    .ip = addr.sin_addr,
                    .port = addr.sin_port,
                };
                handle_msg(sa, pkt.get());
            }
        }
    }

exit:
    free(buf);
    vTaskDelete(nullptr);
}

void Protocol::handle_msg(const SockAddr& addr, rbjson::Object* pkt) {
    const auto cmd = pkt->getString("c");

    if (cmd == "discover") {
        std::unique_ptr<Object> res(new Object());
        res->set("c", "found");
        res->set("owner", m_owner);
        res->set("name", m_name);
        res->set("desc", m_desc);

        const auto str = res->str();
        send(addr, str.c_str(), str.size());
        return;
    }

    if (!pkt->contains("n")) {
        ESP_LOGE(TAG, "packet does not have counter!");
        return;
    }

    const int counter = pkt->getInt("n");
    if (counter == -1) {
        m_read_counter = 0;
        m_mutex.lock();
        m_write_counter = 0;
        m_mutex.unlock();
    } else if (counter < m_read_counter && m_read_counter - counter < 25) {
        return;
    } else {
        m_read_counter = counter;
    }

    if (m_possessed_addr.port == 0 || cmd == "possess") {
        m_mutex.lock();
        if (m_possessed_addr.ip.s_addr != addr.ip.s_addr || m_possessed_addr.port != addr.port) {
            m_possessed_addr = addr;
        }
        m_mustarrive_e = 0;
        m_mustarrive_f = 0xFFFFFFFF;
        m_write_counter = -1;
        m_read_counter = -1;
        m_mutex.unlock();

        m_mustarrive_mutex.lock();
        for (auto it : m_mustarrive_queue) {
            delete it.pkt;
        }
        m_mustarrive_queue.clear();
        m_mustarrive_mutex.unlock();
    }

    if (pkt->contains("f")) {
        {
            std::unique_ptr<Object> resp(new Object);
            resp->set("c", cmd);
            resp->set("f", pkt->getInt("f"));
            send(addr, resp.get());
        }

        int f = pkt->getInt("f");
        if (f <= m_mustarrive_f && m_mustarrive_f != 0xFFFFFFFF) {
            return;
        } else {
            m_mustarrive_f = f;
        }
    } else if (pkt->contains("e")) {
        uint32_t e = pkt->getInt("e");
        m_mustarrive_mutex.lock();
        for (auto itr = m_mustarrive_queue.begin(); itr != m_mustarrive_queue.end(); ++itr) {
            if ((*itr).id == e) {
                delete (*itr).pkt;
                m_mustarrive_queue.erase(itr);
                break;
            }
        }
        m_mustarrive_mutex.unlock();
        return;
    }

    if (cmd == "possess") {
        ESP_LOGI(TAG, "We are possessed!");
        send_log("The device %s has been possessed!\n", m_name);
    }

    if (m_callback != NULL) {
        m_callback(cmd, pkt);
    }
}

}; // namespace rb
