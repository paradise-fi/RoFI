#pragma once

#include <freertos/FreeRTOS.h>
#include <functional>
#include <mutex>
#include <stdarg.h>
#include <string>
#include <vector>

#include <lwip/arch.h>
#include <lwip/sockets.h>

#include "rbjson.h"

namespace rb {

#define RBPROTOCOL_PORT 42424 //!< The default RBProtocol port

#define RBPROTOCOL_AXIS_MIN (-32767) //!< Minimal value of axes in "joy" command
#define RBPROTOCOL_AXIS_MAX (32767) //!< Maximal value of axes in "joy" command

class Protocol;

/**
 * \brief Class that manages the RBProtocol communication
 */
class Protocol {
public:
    typedef std::function<void(const std::string& cmd, rbjson::Object* pkt)> callback_t;

    /**
     * The onPacketReceivedCallback is called when a packet arrives.
     * It runs on a separate task, only single packet is processed at a time.
     */
    Protocol(const char* owner, const char* name, const char* description, callback_t callback = nullptr);
    ~Protocol();

    void start(uint16_t port = RBPROTOCOL_PORT); //!< Start listening for UDP packets on port
    void stop(); //!< Stop listening

    /**
     * \brief Send command cmd with params, without making sure it arrives.
     *
     * If you pass the params object, you are responsible for its deletion.
     */
    void send(const char* cmd, rbjson::Object* params = NULL);

    /**
     * \brief Send command cmd with params and make sure it arrives.
     *
     * If you pass the params object, it has to be heap-allocated and
     * RbProtocol becomes its owner - you MUST NOT delete it.
     * 
     * \return id of the mustarrive packet, you can use it in is_mustarrive_complete.
     *         Returns UINT32_MAX if the sending failed.
     */
    uint32_t send_mustarrive(const char* cmd, rbjson::Object* params = NULL);

    void send_log(const char* fmt, ...); //!< Send a message to the android app
    void send_log(const char* fmt, va_list args); //!< Send a message to the android app
    void send_log(const std::string& str); //!< Send a message to the android app

    bool is_possessed() const; //!< Returns true of the device is possessed (somebody connected to it)

    bool is_mustarrive_complete(uint32_t id) const;

    TaskHandle_t getTaskSend() const { return m_task_send; }
    TaskHandle_t getTaskRecv() const { return m_task_recv; }

private:
    struct MustArrive {
        rbjson::Object* pkt;
        uint32_t id;
        int16_t attempts;
    };

    struct SockAddr {
        struct in_addr ip;
        uint16_t port;
    };

    struct QueueItem {
        SockAddr addr;
        char* buf;
        uint16_t size;
    };

    bool get_possessed_addr(SockAddr& addr);

    static void send_task_trampoline(void* ctrl);
    void send_task();
    void resend_mustarrive_locked();

    static void recv_task_trampoline(void* ctrl);
    void recv_task();

    void send(const SockAddr& addr, const char* command, rbjson::Object* obj);
    void send(const SockAddr& addr, rbjson::Object* obj);
    void send(const SockAddr& addr, const char* buf);
    void send(const SockAddr& addr, const char* buf, size_t size);

    void handle_msg(const SockAddr& addr, rbjson::Object* pkt);

    const char* m_owner;
    const char* m_name;
    const char* m_desc;

    callback_t m_callback;

    TaskHandle_t m_task_send;
    TaskHandle_t m_task_recv;

    int m_socket;
    int32_t m_read_counter;
    int32_t m_write_counter;
    SockAddr m_possessed_addr;
    QueueHandle_t m_sendQueue;
    mutable std::mutex m_mutex;

    uint32_t m_mustarrive_e;
    uint32_t m_mustarrive_f;
    std::vector<MustArrive> m_mustarrive_queue;
    mutable std::mutex m_mustarrive_mutex;
};

};
