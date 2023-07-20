#pragma once

#include "../widgets/input.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Input : public Widget, public BuilderMixin<Input, gridui::Input> {
    static const char* name() { return "Input"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Input& text(const std::string& text) {
        extra().set("text", text);
        return *this;
    }

    Input& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Input& type(const std::string& type) {
        extra().set("type", type);
        return *this;
    }

    Input& disabled(bool disabled) {
        extra().set("disabled", new rbjson::Bool(disabled));
        return *this;
    }

    /** @ingroup event
     */
    Input& onChanged(callback_t cb) {
        addCallback("changed", cb);
        return *this;
    }
};

};
};
