#pragma once

#include <jac/link/stream.h>
#include <jac/device/logger.h>

#include <atomic>
#include "driver/usb_serial_jtag.h"


class JtagStream : public jac::Duplex {
    std::function<void()> _onData;
    std::thread _eventThread;
    std::atomic<bool> _stopThread = false;
    std::deque<uint8_t> _buffer;
public:
    JtagStream(uint32_t rxBufferSize, uint32_t txBufferSize) {
        usb_serial_jtag_driver_config_t config = {
            .tx_buffer_size = txBufferSize,
            .rx_buffer_size = rxBufferSize,
        };

        ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));
    }
    JtagStream(JtagStream&&) = delete;
    JtagStream(const JtagStream&) = delete;
    JtagStream& operator=(JtagStream&&) = delete;
    JtagStream& operator=(const JtagStream&) = delete;

    void start() {
        _eventThread = std::thread([this]() noexcept {
            std::array<uint8_t, 64> buffer;
            while (!_stopThread) {
                int len = usb_serial_jtag_read_bytes(buffer.data(), buffer.size(), portMAX_DELAY);
                _buffer.insert(_buffer.end(), buffer.begin(), buffer.begin() + len);

                if (len > 0) {
                    if (_onData) {
                        _onData();
                    }
                }
            }
        });
    }

    bool put(uint8_t c) override {
        auto res = usb_serial_jtag_write_bytes(&c, 1, 10) == 1;
        return res;
    }

    size_t write(std::span<const uint8_t> data) override {
        auto res = usb_serial_jtag_write_bytes(data.data(), data.size(), 10);
        return res;
    }

    int get() override {
        if (_buffer.empty()) {
            return -1;
        }
        uint8_t c = _buffer.front();
        _buffer.pop_front();
        return c;
    }

    size_t read(std::span<uint8_t> data) override {
        size_t len = std::min(data.size(), _buffer.size());
        std::copy_n(_buffer.begin(), len, data.begin());
        _buffer.erase(_buffer.begin(), _buffer.begin() + len);
        return len;
    }

    bool flush() override {
        return true;
    }

    void onData(std::function<void(void)> callback) override {
        _onData = callback;
    }

    ~JtagStream() override {
        _stopThread = true;
        _eventThread.join();
        usb_serial_jtag_driver_uninstall();
    }
};
