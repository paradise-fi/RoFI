#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include <stdint.h>

#include "rbjson.h"

#include "../widgets/widget.h"

/**
 *  @defgroup event Event handlers
 *  This group contains all possible event handlers
 */

/**
 *  @defgroup widgets_builder Builder widgets
 *  Classes in this module are used to construct the layout.
 */

namespace gridui {

class _GridUi;

namespace builder {

template <typename Self, typename Constructed>
class BuilderMixin {
    // The Self is not fully defined yet here, so the check won't compile.
    //static_assert(std::is_base_of<Widget, Self>::value, "Self must inherit from Widget.");
    static_assert(std::is_base_of<gridui::Widget, Constructed>::value, "Constructed must inherit from gridui::Widget.");

    friend class gridui::_GridUi;

public:
    typedef std::function<void(Constructed&)> callback_t;

    Self& css(const std::string& key, const std::string& value) {
        auto& s = self();
        s.style().set(key, value);
        return s;
    }

    Constructed finish() {
        return Constructed(&self().m_state);
    }

protected:
    void addCallback(const std::string& name, callback_t cb) {
        auto& all = self().m_state.callbacks();

        auto old = all.find(name);
        if (old != all.end()) {
            delete static_cast<callback_t*>(old->second);
        }

        auto* cbHeap = new callback_t(cb);
        all[name] = static_cast<void*>(cbHeap); // fuj
    }

private:
    Self& self() { return *static_cast<Self*>(this); }
    const Self& self() const { return *static_cast<Self*>(this); }

    static void callbackTrampoline(void* cb, WidgetState* state) {
        Constructed w(state);
        (*static_cast<callback_t*>(cb))(w);
    }
};

class Widget {
    friend class gridui::_GridUi;

    template <typename Self, typename Finished>
    friend class BuilderMixin;

public:
    Widget(Widget&& o) noexcept;
    virtual ~Widget();

protected:
    Widget(const char* type, WidgetState& state);

    virtual void serialize(std::stringstream& ss);

    rbjson::Object& extra();
    rbjson::Object& style();

    WidgetState& m_state;

private:
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    const char* m_type;
    rbjson::Object* m_style;
};

};
};
