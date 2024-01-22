#pragma once

#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <functional>
#include <iostream>
#include <optional>
#include <variant>
#include <tuple>

template<class Next>
class FreeRTOSEventQueueFeature : public Next {
public:
    struct Event {
        std::variant<
            std::monostate,
            std::function<void()>*,
            std::tuple<void(*)(void*), void*>
        > event = std::monostate();

        Event() : event(std::monostate()) {}

        Event(std::function<void()> func) : event(new std::function<void()>(std::move(func))) {}
        Event(void(*func)(void*), void* arg) : event(std::make_tuple(func, arg)) {}

        Event& operator=(const Event& other) = delete;
        Event(const Event& other) = delete;
        Event& operator=(Event&& other) {
            std::swap(event, other.event);
            return *this;
        }
        Event(Event&& other) {
            *this = std::move(other);
        }

        ~Event() {
            if (std::holds_alternative<std::function<void()>*>(event)) {
                delete std::get<std::function<void()>*>(event);
            }
        }

        void operator()() {
            if (std::holds_alternative<std::function<void()>*>(event)) {
                auto func = std::get<std::function<void()>*>(event);
                if (func) {
                    (*func)();
                }
            }
            else if (std::holds_alternative<std::tuple<void(*)(void*), void*>>(event)) {
                auto [func, arg] = std::get<std::tuple<void(*)(void*), void*>>(event);
                if (func) {
                    func(arg);
                }
            }
            else {
                // empty event
            }
        }

        operator bool() const {
            return !std::holds_alternative<std::monostate>(event);
        }

        void release() {
            event = std::monostate();
        }
    };
private:
    QueueHandle_t _eventQueue;

    Event dequeue(bool wait) {
        Event e;
        if (xQueueReceive(_eventQueue, &e, wait ? portMAX_DELAY : 0)) {
            return e;
        }
        else {
            return Event();
        }
    }
public:

    FreeRTOSEventQueueFeature() {
        _eventQueue = xQueueCreate(64, sizeof(Event));
    }

    /**
     * @brief Check the event queue and return the first event
     * @param wait Wait for event if no event is available
     * @return Event or std::nullopt if no event is available
     */
    std::optional<Event> getEvent(bool wait) {
        auto e(dequeue(wait));
        if (!e) {
            return std::nullopt;
        }
        return e;
    }

    /**
     * @brief Schedule an event to be run
     * @param func Function to be run
     */
    void scheduleEvent(std::function<void()> func) {
        Event e(std::move(func));
        auto res = xQueueSend(_eventQueue, &e, portMAX_DELAY);
        if (res != pdPASS) {
            // TODO: handle error
            return;
        }
        e.release();
    }

    /**
     * @brief Schedule an event to be run
     * @param func Function to be run
     */
    void scheduleEvent(void(*func)(void*), void* arg) {
        Event e(func, arg);
        auto res = xQueueSend(_eventQueue, &e, portMAX_DELAY);
        if (res != pdPASS) {
            // TODO: handle error
            return;
        }
        e.release();
    }

    /**
     * @brief Schedule an event to be run from ISR
     * @param func Function to be run
     */
    void scheduleEventISR(void(*func)(void*), void* arg) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        Event e(func, arg);
        auto res = xQueueSendFromISR(_eventQueue, &e, &xHigherPriorityTaskWoken);
        if (res != pdPASS) {
            // TODO: handle error
            return;
        }
        e.release();
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }

    /**
     * @brief Wake up event loop if it is waiting for events
     */
    void notifyEventLoop() {
        Event e;
        xQueueSend(_eventQueue, &e, 0);
    }

    ~FreeRTOSEventQueueFeature() {
        notifyEventLoop();
        while (dequeue(false)) {}
        vQueueDelete(_eventQueue);
    }
};
