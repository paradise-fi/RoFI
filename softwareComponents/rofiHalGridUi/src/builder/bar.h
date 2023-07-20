#pragma once

#include "../widgets/bar.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Bar : public Widget, public BuilderMixin<Bar, gridui::Bar> {
    static const char* name() { return "Bar"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Bar& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Bar& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    Bar& min(float min) {
        extra().set("min", min);
        return *this;
    }

    Bar& max(float max) {
        extra().set("max", max);
        return *this;
    }

    Bar& value(float value) {
        extra().set("value", value);
        return *this;
    }

    Bar& showValue(bool showValue) {
        extra().set("showValue", new rbjson::Bool(showValue));
        return *this;
    }
};

};
};
