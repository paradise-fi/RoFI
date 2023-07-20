#pragma once

#include "../widgets/text.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Text : public Widget, public BuilderMixin<Text, gridui::Text> {
    static const char* name() { return "Text"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Text& text(const std::string& text) {
        extra().set("text", text);
        return *this;
    }

    Text& fontSize(float fontSize) {
        extra().set("fontSize", fontSize);
        return *this;
    }

    Text& color(const std::string& color) {
        extra().set("color", color);
        return *this;
    }

    Text& background(const std::string& background) {
        extra().set("background", background);
        return *this;
    }

    Text& align(const std::string& align) {
        extra().set("align", align);
        return *this;
    }

    Text& valign(const std::string& valign) {
        extra().set("valign", valign);
        return *this;
    }

    Text& prefix(const std::string& prefix) {
        extra().set("prefix", prefix);
        return *this;
    }

    Text& suffix(const std::string& suffix) {
        extra().set("suffix", suffix);
        return *this;
    }
};

};
};
