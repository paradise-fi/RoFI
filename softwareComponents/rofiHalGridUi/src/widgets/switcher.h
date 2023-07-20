#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Switcher : public Widget {
    template <typename Self, typename Finished>
    friend class builder::BuilderMixin;

    using Widget::Widget;

public:
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

    int value() const {
        return data().getInt("value");
    }

    void setMin(int min) {
        m_state->set("min", new rbjson::Number(min));
    }

    int min() const {
        return data().getInt("min");
    }

    void setMax(int max) {
        m_state->set("max", new rbjson::Number(max));
    }

    int max() const {
        return data().getInt("max");
    }
};

};
