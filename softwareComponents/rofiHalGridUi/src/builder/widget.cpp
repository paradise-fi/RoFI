#include <esp_log.h>
#include <stdio.h>

#include "widget.h"

#define TAG "UiBuilderWidget"

namespace gridui {
namespace builder {

Widget::Widget(const char* type, WidgetState& state)
    : m_state(state)
    , m_type(type)
    , m_style(nullptr) {
}

Widget::~Widget() {
}

rbjson::Object& Widget::extra() {
    return m_state.data();
}

rbjson::Object& Widget::style() {
    auto* res = extra().getObject("css");
    if (res == nullptr) {
        res = new rbjson::Object;
        extra().set("css", res);
    }
    return *res;
}

void Widget::serialize(std::stringstream& ss) {
    ss << "{";
    {
        ss << "\"uuid\":" << m_state.uuid() << ",";
        ss << "\"type\":\"" << m_type << "\",";
        ss << "\"state\":";
        extra().serialize(ss);
    }
    ss << "}";
}
};

};
