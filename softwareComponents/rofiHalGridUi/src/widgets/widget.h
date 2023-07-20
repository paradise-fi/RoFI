#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <stdint.h>

#include "rbjson.h"

namespace gridui {

class _GridUi;

namespace builder {
class Widget;

template <typename Self, typename Finished>
class BuilderMixin;
};

/**
 *  @defgroup widgets_constructed Layout widgets
 *  Classes in this module are used to modify state of the already constructed Layout.
 */

class WidgetState {
    friend class gridui::builder::Widget;
    friend class gridui::_GridUi;

    template <typename Self, typename Finished>
    friend class builder::BuilderMixin;

    typedef void (*cb_trampoline_t)(void*, WidgetState*);

public:
    WidgetState(uint16_t uuid, float x, float y, float w, float h, uint16_t tab, cb_trampoline_t cb_trampoline);

    uint16_t uuid() const { return m_uuid; }
    const rbjson::Object& data() const { return m_data; }

    bool set(const std::string& key, rbjson::Value* value);
    bool setInnerObjectProp(const std::string& objectName, const std::string& propertyName,
        rbjson::Value* value);

    void markChanged(const std::string& key);

private:
    WidgetState(const WidgetState&) = delete;
    WidgetState& operator=(const WidgetState&) = delete;

    rbjson::Object& data() { return m_data; }

    void update(rbjson::Object* other) {
        m_mutex.lock();
        for (auto itr : other->members()) {
            m_data.set(itr.first, itr.second->copy());
            markGlobalChangedLocked(itr.first);
        }
        m_mutex.unlock();
    }

    std::map<std::string, void*>& callbacks() {
        if (!m_callbacks) {
            m_callbacks.reset(new std::map<std::string, void*>);
        }
        return *m_callbacks.get();
    }

    void call(const std::string& event) {
        if (!m_callbacks)
            return;
        auto itr = m_callbacks->find(event);
        if (itr != m_callbacks->end()) {
            (*m_cb_trampoline)(itr->second, this);
        }
    }

    void markChangedLocked(const std::string& key);
    void markGlobalChangedLocked(const std::string& key);
    inline bool wasChangedInTickLocked(const std::string& key) const;

    bool popChanges(rbjson::Object& state);
    bool remarkAllChanges();

    rbjson::Object m_data;
    const cb_trampoline_t m_cb_trampoline;
    std::unique_ptr<std::map<std::string, void*>> m_callbacks;

    mutable std::mutex m_mutex;

    const uint16_t m_uuid;

    uint16_t m_bloom_global;
    uint16_t m_bloom_tick;
};

class Widget {
public:
    Widget()
        : m_state(&emptyState) {
    }

    Widget(const Widget&& o)
        : m_state(o.m_state) {
    }

    Widget& operator=(const Widget&& o) {
        m_state = o.m_state;
        return *this;
    }

    uint16_t uuid() const {
        return m_state->uuid();
    }

    void setWidgetX(float val) {
        m_state->set("x", new rbjson::Number(val));
    }

    float widgetX() const {
        return data().getDouble("x");
    }

    void setWidgetY(float val) {
        m_state->set("y", new rbjson::Number(val));
    }

    float widgetY() const {
        return data().getDouble("y");
    }

    void setWidgetW(float val) {
        m_state->set("w", new rbjson::Number(val));
    }

    float widgetW() const {
        return data().getDouble("w");
    }

    void setWidgetH(float val) {
        m_state->set("h", new rbjson::Number(val));
    }

    float widgetH() const {
        return data().getDouble("h");
    }

    void setWidgetTab(uint16_t tab) {
        m_state->set("tab", new rbjson::Number(tab));
    }

    uint16_t widgetTab() const {
        return data().getInt("tab");
    }

    void setCss(const std::string& propertyName, const std::string& value) {
        m_state->setInnerObjectProp("css", propertyName, new rbjson::String(value));
    }

    std::string css(const std::string& propertyName) const {
        auto* css = data().getObject("css");
        if (css == nullptr)
            return "";
        return css->getString(propertyName);
    }

protected:
    Widget(WidgetState* state)
        : m_state(state) {
    }

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    const rbjson::Object& data() const { return static_cast<const WidgetState*>(m_state)->data(); }

    WidgetState* m_state;

private:
    static WidgetState emptyState;
};

};
