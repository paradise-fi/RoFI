#pragma once

#include "../widgets/camera.h"
#include "widget.h"

namespace gridui {
namespace builder {

/** @ingroup widgets_builder
*/
class Camera : public Widget, public BuilderMixin<Camera, gridui::Camera> {
    static const char* name() { return "Camera"; }

    friend class gridui::_GridUi;
    using Widget::Widget;

public:

    Camera& rotation(float rotation) {
        extra().set("rotation", rotation);
        return *this;
    }

    Camera& clip(bool clip) {
        extra().set("clip", new rbjson::Bool(clip));
        return *this;
    }

};

};
};
