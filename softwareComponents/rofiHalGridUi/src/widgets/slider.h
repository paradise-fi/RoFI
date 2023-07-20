#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Slider : public Widget {
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

    void setFontSize(float fontSize) {
        m_state->set("fontSize", new rbjson::Number(fontSize));
    }

    float fontSize() const {
        return data().getDouble("fontSize");
    }

    void setMin(float min) {
        m_state->set("min", new rbjson::Number(min));
    }

    float min() const {
        return data().getDouble("min");
    }

    void setMax(float max) {
        m_state->set("max", new rbjson::Number(max));
    }

    float max() const {
        return data().getDouble("max");
    }

    void setValue(float value) {
        m_state->set("value", new rbjson::Number(value));
    }

    float value() const {
        return data().getDouble("value");
    }

    void setPrecision(float precision) {
        m_state->set("precision", new rbjson::Number(precision));
    }

    float precision() const {
        return data().getDouble("precision");
    }

    void setShowValue(bool showValue) {
        m_state->set("showValue", new rbjson::Bool(showValue));
    }

    bool showValue() const {
        return data().getBool("showValue");
    }
};

};
