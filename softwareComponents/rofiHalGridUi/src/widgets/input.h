#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Input : public Widget {
    template <typename Self, typename Finished>
    friend class builder::BuilderMixin;

    using Widget::Widget;

public:
    void setText(const std::string& text) {
        m_state->set("text", new rbjson::String(text));
    }

    std::string text() const {
        return data().getString("text");
    }

    void setColor(const std::string& color) {
        m_state->set("color", new rbjson::String(color));
    }

    std::string color() const {
        return data().getString("color");
    }

    void setType(const std::string& type) {
        m_state->set("type", new rbjson::String(type));
    }

    std::string type() const {
        return data().getString("type");
    }

    void setDisabled(bool disabled) {
        m_state->set("disabled", new rbjson::Bool(disabled));
    }

    bool disabled() const {
        return data().getBool("disabled");
    }

};

};
