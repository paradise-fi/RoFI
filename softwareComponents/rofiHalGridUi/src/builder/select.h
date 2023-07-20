#pragma once

#include "../widgets/select.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Select : public Widget, public BuilderMixin<Select, gridui::Select> {
    static const char* name() { return "Select"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Select& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Select& background(const std::string& background) {
        extra().set("background", background);
        return *this;
    }

    Select& disabled(bool disabled) {
        extra().set("disabled", new rbjson::Bool(disabled));
        return *this;
    }

    Select& options(const std::vector<std::string>& options) {
        std::string out = "";
        for (auto& option : options) {
            out.append(option);
            out.push_back(',');
        }
        if (out.size() > 0)
            out.pop_back();
        extra().set("options", new rbjson::String(out));
        return *this;
    }

    Select& options(const std::string& options) {
        extra().set("options", new rbjson::String(options));
        return *this;
    }

    Select& selectedIndex(int index) {
        extra().set("selectedIndex", new rbjson::Number(index));
        return *this;
    }

    /** @ingroup event
     */
    Select& onChanged(callback_t cb) {
        addCallback("changed", cb);
        return *this;
    }
};

};
};
