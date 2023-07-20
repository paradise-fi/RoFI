#pragma once

#include "../widgets/circle.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Circle : public Widget, public BuilderMixin<Circle, gridui::Circle> {
    static const char* name() { return "Circle"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Circle& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Circle& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    Circle& min(float min) {
        extra().set("min", min);
        return *this;
    }

    Circle& max(float max) {
        extra().set("max", max);
        return *this;
    }

    Circle& lineWidth(float lineWidth) {
        extra().set("lineWidth", lineWidth);
        return *this;
    }

    Circle& valueStart(float valueStart) {
        extra().set("valueStart", valueStart);
        return *this;
    }

    Circle& value(float value) {
        extra().set("value", value);
        return *this;
    }

    Circle& showValue(bool showValue) {
        extra().set("showValue", new rbjson::Bool(showValue));
        return *this;
    }

};

};
};
