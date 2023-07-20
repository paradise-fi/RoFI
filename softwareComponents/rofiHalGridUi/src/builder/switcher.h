#pragma once

#include "../widgets/switcher.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Switcher : public Widget, public BuilderMixin<Switcher, gridui::Switcher> {
    static const char* name() { return "Switcher"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Switcher& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    Switcher& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Switcher& min(int min) {
        extra().set("min", min);
        return *this;
    }

    Switcher& max(int max) {
        extra().set("max", max);
        return *this;
    }
};

};
};
