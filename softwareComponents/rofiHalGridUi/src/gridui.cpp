#include <esp_log.h>
#include <esp_timer.h>
#include <stdio.h>

#include "gridui.h"
#include "rbprotocol.h"
#include "rbwebserver.h"

namespace gridui {

_GridUi UI;

_GridUi::_GridUi()
    : m_protocol(nullptr)
    , m_state_mustarrive_id(UINT32_MAX)
    , m_states_modified(false) {
}

_GridUi::~_GridUi() {
}

void _GridUi::begin(rb::Protocol* protocol, int cols, int rows, bool enableSplitting) {
    std::lock_guard<std::mutex> l(m_states_mu);
    if (m_protocol != nullptr) {
        ESP_LOGE("GridUI", "begin() called more than once!");
        return;
    }

    m_protocol = protocol;

    m_layout.reset(new rbjson::Object);
    m_layout->set("cols", cols);
    m_layout->set("rows", rows);
    m_layout->set("enableSplitting", new rbjson::Bool(enableSplitting));
}

uint16_t _GridUi::generateUuidLocked() const {
    while (1) {
        const uint32_t rnd = esp_random();
        if (checkUuidFreeLocked(rnd & 0xFFFF))
            return rnd & 0xFFFF;
        if (checkUuidFreeLocked(rnd >> 16))
            return rnd >> 16;
    }
}

void _GridUi::commit() {
    std::lock_guard<std::mutex> l(m_states_mu);
    if (!m_layout) {
        ESP_LOGE("GridUI", "commit() called with no layout prepared!");
        return;
    }

    std::vector<char> layout_json;
    {
        std::stringstream ss;
        m_layout->serialize(ss);
        m_layout.reset();

        ss.seekp(-1, std::stringstream::cur);

        ss << ",\"widgets\": [";
        for (size_t i = 0; i < m_widgets.size(); ++i) {
            if (i != 0) {
                ss << ",";
            }
            auto& w = m_widgets[i];
            w->serialize(ss);
            w.reset();
        }
        m_widgets.clear();
        m_widgets.shrink_to_fit();
        m_states.shrink_to_fit();

        ss << "]}";

        layout_json.resize(((size_t)ss.tellp()) + 1);
        ss.get(layout_json.data(), layout_json.size());
    }

    ESP_ERROR_CHECK(rb_web_add_file("layout.json", layout_json.data(), layout_json.size() - 1));

    esp_timer_create_args_t args = {
        .callback = stateChangeTask,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "gridui_state",
    };
    esp_timer_handle_t timer;
    esp_timer_create(&args, &timer);
    esp_timer_start_periodic(timer, 50 * 1000);
}

bool _GridUi::handleRbPacket(const std::string& cmd, rbjson::Object* pkt) {
    if (cmd == "_gev") {
        m_states_mu.lock();
        auto* state = stateByUuidLocked(pkt->getInt("id"));
        if (state == nullptr) {
            m_states_mu.unlock();
            return true;
        }

        auto* st = pkt->getObject("st");
        if (st != nullptr) {
            state->update(st);
        }
        m_states_mu.unlock();

        state->call(pkt->getString("ev"));
    } else if (cmd == "_gall") {
        bool changed = false;
        std::lock_guard<std::mutex> l(m_states_mu);
        for (auto& itr : m_states) {
            if (itr->remarkAllChanges())
                changed = true;
        }
        if (changed) {
            m_states_modified = true;
        }
        m_tab_changed = true;
    } else {
        return false;
    }
    return true;
}

void _GridUi::changeTab(uint16_t index) {
    std::lock_guard<std::mutex> k(m_tab_mu);
    m_tab_changed = true;
    m_tab = index;
}

void _GridUi::stateChangeTask(void* selfVoid) {
    auto* self = (_GridUi*)selfVoid;

    auto* prot = self->protocol();
    if (prot == nullptr || !prot->is_possessed())
        return;

    if (!prot->is_mustarrive_complete(self->m_state_mustarrive_id))
        return;

    if (self->m_states_modified.exchange(false)) {
        std::unique_ptr<rbjson::Object> pkt(new rbjson::Object);
        {
            std::lock_guard<std::mutex>(self->m_states_mu);
            char buf[6];
            std::unique_ptr<rbjson::Object> state(new rbjson::Object);

            const size_t size = self->m_states.size();
            for (size_t i = 0; i < size; ++i) {
                auto& s = self->m_states[i];
                if (s->popChanges(*state.get())) {
                    snprintf(buf, sizeof(buf), "%d", (int)s->uuid());
                    pkt->set(buf, state.release());
                    state.reset(new rbjson::Object);
                }
            }
        }
        self->m_state_mustarrive_id = prot->send_mustarrive("_gst", pkt.release());
    }

    if (self->m_tab_changed.exchange(false)) {
        std::lock_guard<std::mutex> lock(self->m_tab_mu);
        std::unique_ptr<rbjson::Object> pkt(new rbjson::Object);

        pkt->set("tab", self->m_tab);

        self->m_state_mustarrive_id = prot->send_mustarrive("_gtb", pkt.release());
    }
}
};
