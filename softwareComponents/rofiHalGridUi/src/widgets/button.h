#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Button : public Widget {
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

    void setFontSize(float fontSize) {
        m_state->set("fontSize", new rbjson::Number(fontSize));
    }

    float fontSize() const {
        return data().getDouble("fontSize");
    }

    void setColor(const std::string& color) {
        m_state->set("color", new rbjson::String(color));
    }

    std::string color() const {
        return data().getString("color");
    }

    void setBackground(const std::string& background) {
        m_state->set("background", new rbjson::String(background));
    }

    std::string background() const {
        return data().getString("background");
    }

    void setAlign(const std::string& align) {
        m_state->set("align", new rbjson::String(align));
    }

    std::string align() const {
        return data().getString("align");
    }

    void setValign(const std::string& valign) {
        m_state->set("valign", new rbjson::String(valign));
    }

    std::string valign() const {
        return data().getString("valign");
    }

    void setNumber(float number) {
        rbjson::Number n(number);
        setText(n.str());
    }

    bool pressed() const {
        return data().getBool("pressed");
    }

    void setDisabled(bool disabled) {
        m_state->set("disabled", new rbjson::Bool(disabled));
    }

    bool disabled() const {
        return data().getBool("disabled");
    }
};

};
