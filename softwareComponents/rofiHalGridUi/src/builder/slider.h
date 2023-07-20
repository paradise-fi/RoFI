#pragma once

#include "../widgets/slider.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Slider : public Widget, public BuilderMixin<Slider, gridui::Slider> {
    static const char* name() { return "Slider"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Slider& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Slider& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    Slider& min(float min) {
        extra().set("min", min);
        return *this;
    }

    Slider& max(float max) {
        extra().set("max", max);
        return *this;
    }

    Slider& value(float value) {
        extra().set("value", value);
        return *this;
    }

    Slider& precision(float precision) {
        extra().set("precision", precision);
        return *this;
    }

    Slider& showValue(bool showValue) {
        extra().set("showValue", new rbjson::Bool(showValue));
        return *this;
    }

    /** @ingroup event
     */
    Slider& onChanged(callback_t cb) {
        addCallback("changed", cb);
        return *this;
    }
};

};
};
