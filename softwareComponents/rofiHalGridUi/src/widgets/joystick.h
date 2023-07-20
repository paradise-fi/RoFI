#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Joystick : public Widget {
    template <typename Self, typename Finished>
    friend class builder::BuilderMixin;

    using Widget::Widget;

public:
    void setColor(const std::string& color) {
        m_state->set("color", new rbjson::String(color));
    }

    std::string color() const {
        return data().getString("color");
    }

    void setKeys(const std::string& keys) {
        m_state->set("keys", new rbjson::String(keys));
    }

    std::string keys() const {
        return data().getString("keys");
    }

    void setText(const std::string& text) {
        m_state->set("text", new rbjson::String(text));
    }

    std::string text() const {
        return data().getString("text");
    }

    int32_t x() const {
        return data().getInt("jx");
    }

    int32_t y() const {
        return data().getInt("jy");
    }
};

};
