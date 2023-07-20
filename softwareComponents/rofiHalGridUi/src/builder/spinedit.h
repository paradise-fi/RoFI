#pragma once

#include "../widgets/spinedit.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class SpinEdit : public Widget, public BuilderMixin<SpinEdit, gridui::SpinEdit> {
    static const char* name() { return "SpinEdit"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    SpinEdit& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    SpinEdit& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    SpinEdit& value(float value) {
        extra().set("value", value);
        return *this;
    }

    SpinEdit& step(float step) {
        extra().set("step", step);
        return *this;
    }

    SpinEdit& precision(float precision) {
        extra().set("precision", precision);
        return *this;
    }

    /** @ingroup event
     */
    SpinEdit& onChanged(callback_t cb) {
        addCallback("changed", cb);
        return *this;
    }
};

};
};
