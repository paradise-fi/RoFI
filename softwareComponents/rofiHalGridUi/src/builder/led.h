#pragma once

#include "../widgets/led.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Led : public Widget, public BuilderMixin<Led, gridui::Led> {
    static const char* name() { return "Led"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Led& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Led& on(bool on) {
        extra().set("on", new rbjson::Bool(on));
        return *this;
    }
};

};
};
