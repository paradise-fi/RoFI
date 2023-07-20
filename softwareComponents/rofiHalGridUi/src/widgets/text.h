#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Text : public Widget {
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

    void setPrefix(const std::string& prefix) {
        m_state->set("prefix", new rbjson::String(prefix));
    }

    std::string prefix() const {
        return data().getString("prefix");
    }

    void setSuffix(const std::string& suffix) {
        m_state->set("suffix", new rbjson::String(suffix));
    }

    std::string suffix() const {
        return data().getString("suffix");
    }

    void setNumber(float number) {
        rbjson::Number n(number);
        setText(n.str());
    }
};

};
