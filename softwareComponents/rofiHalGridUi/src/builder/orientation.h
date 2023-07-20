#pragma once

#include "../widgets/orientation.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Orientation : public Widget, public BuilderMixin<Orientation, gridui::Orientation> {
    static const char* name() { return "Orientation"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Orientation& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    /** @ingroup event
     */
    Orientation& onPositionChanged(callback_t cb) {
        addCallback("pos", cb);
        return *this;
    }
};

};
};
