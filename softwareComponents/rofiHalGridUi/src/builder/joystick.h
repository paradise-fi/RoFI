#pragma once

#include "../widgets/joystick.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Joystick : public Widget, public BuilderMixin<Joystick, gridui::Joystick> {
    static const char* name() { return "Joystick"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Joystick& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Joystick& keys(const std::string& keys) {
        extra().set("keys", keys);
        return *this;
    }

    Joystick& text(const std::string& text) {
        extra().set("text", text);
        return *this;
    }

    Joystick& keys(char forward, char left, char backwards, char right, char click = 0) {
        const char keys[6] = { forward, left, backwards, right, click, 0 };
        extra().set("keys", keys);
        return *this;
    }

    /** @ingroup event
     */
    Joystick& onClick(callback_t cb) {
        addCallback("click", cb);
        return *this;
    }

    /** @ingroup event
     */
    Joystick& onPositionChanged(callback_t cb) {
        addCallback("pos", cb);
        return *this;
    }
};

};
};
