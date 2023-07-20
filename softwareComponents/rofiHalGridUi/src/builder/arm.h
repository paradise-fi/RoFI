#pragma once

#include <stdio.h>

#include "../widgets/arm.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Arm : public Widget, public BuilderMixin<Arm, gridui::Arm> {
    static const char* name() { return "Arm"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:
    Arm& info(std::unique_ptr<rbjson::Object> armInfo) {
        extra().set("info", armInfo.release());
        return *this;
    }

    Arm& onPositionChanged(callback_t cb) {
        addCallback("pos", cb);
        return *this;
    }

    Arm& onGrab(callback_t cb) {
        addCallback("grab", cb);
        return *this;
    }

protected:
    virtual void serialize(std::stringstream& ss) {
        Widget::serialize(ss);
        extra().remove("info");
    }
};

};
};
