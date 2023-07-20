#pragma once

#include "../widgets/button.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Button : public Widget, public BuilderMixin<Button, gridui::Button> {
    static const char* name() { return "Button"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Button& text(const char* text) {
        extra().set("text", text);
        return *this;
    }

    Button& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    Button& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Button& background(const std::string& background) {
        extra().set("background", background);
        return *this;
    }

    Button& align(const std::string& align) {
        extra().set("align", align);
        return *this;
    }

    Button& valign(const std::string& valign) {
        extra().set("valign", valign);
        return *this;
    }

    Button& disabled(bool disabled) {
        extra().set("disabled", new rbjson::Bool(disabled));
        return *this;
    }

    /** @ingroup event
     */
    Button& onPress(callback_t cb) {
        addCallback("press", cb);
        return *this;
    }

    /** @ingroup event
     */
    Button& onRelease(callback_t cb) {
        addCallback("release", cb);
        return *this;
    }
};

};
};
