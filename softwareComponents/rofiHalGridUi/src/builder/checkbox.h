#pragma once

#include "../widgets/checkbox.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Checkbox : public Widget, public BuilderMixin<Checkbox, gridui::Checkbox> {
    static const char* name() { return "Checkbox"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Checkbox& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    Checkbox& checked(bool checked) {
        extra().set("checked", new rbjson::Bool(checked));
        return *this;
    }

    Checkbox& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Checkbox& text(const std::string& text) {
        extra().set("text", text);
        return *this;
    }

    /** @ingroup event
     */
    Checkbox& onChanged(callback_t cb) {
        addCallback("checked", cb);
        return *this;
    }
};

};
};
