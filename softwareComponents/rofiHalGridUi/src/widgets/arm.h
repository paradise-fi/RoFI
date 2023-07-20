#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Arm : public Widget {
    template <typename Self, typename Finished>
    friend class builder::BuilderMixin;

    using Widget::Widget;

public:
    double x() const {
        return data().getDouble("armX");
    }

    double y() const {
        return data().getDouble("armY");
    }
};

};
