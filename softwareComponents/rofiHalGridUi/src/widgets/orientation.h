#pragma once

#include "widget.h"

#include "rbprotocol.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Orientation : public Widget {
    template <typename Self, typename Finished>
    friend class builder::BuilderMixin;

    using Widget::Widget;

public:
    void setColor(const std::string& color) {
        m_state->set("color", new rbjson::String(color));
    }

    std::string color() const {
        return data().getString("color");
    }

    float yaw() const {
        return data().getDouble("oy");
    }

    float pitch() const {
        return data().getDouble("op");
    }

    float roll() const {
        return data().getDouble("or");
    }

    int32_t joystickX() {
        const auto y = yaw();
        if (y >= 0) {
            return y * y * RBPROTOCOL_AXIS_MAX;
        } else {
            return y * y * RBPROTOCOL_AXIS_MAX * -1;
        }
    }

    int32_t joystickY() {
        const auto r = roll();
        if (r >= 0) {
            return r * r * RBPROTOCOL_AXIS_MAX * -1;
        } else {
            return r * r * RBPROTOCOL_AXIS_MAX;
        }
    }
};

};
