#pragma once

#include "widget.h"

#include <cstring>
#include <iostream>
#include <iterator>
#include <vector>

namespace gridui {

/** @ingroup widgets_constructed
*/
class Select : public Widget {
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

    void setBackground(const std::string& background) {
        m_state->set("background", new rbjson::String(background));
    }

    std::string background() const {
        return data().getString("background");
    }

    void setOptions(const std::vector<std::string>& options) {
        std::string out = "";
        for (auto& option : options) {
            out.append(option);
        }
        m_state->set("options", new rbjson::String(out));
    }

    void setOptions(const std::string& options) {
        m_state->set("options", new rbjson::String(options));
    }

    std::vector<std::string> options() const {
        std::vector<std::string> out;
        std::string str = data().getString("options");
        std::string::size_type lastDelim = 0;
        std::string::size_type delim = 0;

        do {
            delim = str.find(',', lastDelim);
            out.push_back(str.substr(lastDelim, delim - lastDelim));
            lastDelim = delim;
        } while (delim != std::string::npos);

        return out;
    }

    void setSelectedIndex(int index) {
        m_state->set("selectedIndex", new rbjson::Number(index));
    }

    int selectedIndex() const {
        return data().getInt("selectedIndex");
    }

    void setDisabled(bool disabled) {
        m_state->set("disabled", new rbjson::Bool(disabled));
    }

    bool disabled() const {
        return data().getBool("disabled");
    }
};

};
