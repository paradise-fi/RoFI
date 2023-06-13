#pragma once

#include <jac/link/stream.h>
#include <jac/device/logger.h>

#include <atomic>
#include "driver/uart.h"


class UartStream : public jac::Duplex {
    uart_port_t _port;
    std::function<void()> _onData;
    QueueHandle_t _eventQueue;
    std::thread _eventThread;
    std::atomic<bool> _stopThread = false;
public:
    UartStream(uart_port_t uartNum, int baudRate, int rxBufferSize, int txBufferSize,
                 int txPin = UART_PIN_NO_CHANGE, int rxPin = UART_PIN_NO_CHANGE,
                 int rtsPin = UART_PIN_NO_CHANGE, int ctsPin = UART_PIN_NO_CHANGE) : _port(uartNum) {
        uart_config_t uart_config = {
            .baud_rate = baudRate,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_DEFAULT
        };

        ESP_ERROR_CHECK(uart_driver_install(uartNum, rxBufferSize, txBufferSize, 10, &_eventQueue, 0));
        ESP_ERROR_CHECK(uart_param_config(uartNum, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(uartNum, txPin, rxPin, rtsPin, ctsPin));
    }
    UartStream(UartStream&&) = delete;
    UartStream(const UartStream&) = delete;
    UartStream& operator=(UartStream&&) = delete;
    UartStream& operator=(const UartStream&) = delete;

    void start() {
        _eventThread = std::thread([this]() noexcept {
            uart_event_t event;
            while (!_stopThread) {
                if (xQueueReceive(_eventQueue, &event, portMAX_DELAY) == pdTRUE) {
                    if (event.type != UART_DATA) {
                        continue;
                    }
                    if (_onData) {
                        _onData();
                    }
                }
            }
        });
    }

    bool put(uint8_t c) override {
        return uart_write_bytes(_port, reinterpret_cast<const char*>(&c), 1) == 1;
    }

    size_t write(std::span<const uint8_t> data) override {
        return uart_write_bytes(_port, reinterpret_cast<const char*>(data.data()), data.size());
    }

    int get() override {
        uint8_t c;
        if (uart_read_bytes(_port, &c, 1, 0) == 1) {
            return c;
        }
        return -1;
    }

    size_t read(std::span<uint8_t> data) override {
        return uart_read_bytes(_port, data.data(), data.size(), 0);
    }

    bool flush() override {
        return uart_wait_tx_done(_port, portMAX_DELAY) == ESP_OK;
    }

    void onData(std::function<void(void)> callback) override {
        _onData = callback;
    }

    ~UartStream() override {
        _stopThread = true;
        _eventThread.join();
        uart_driver_delete(_port);
    }
};
