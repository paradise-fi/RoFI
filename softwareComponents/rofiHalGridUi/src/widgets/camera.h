#pragma once

#include "widget.h"

namespace gridui {

/** @ingroup widgets_constructed
*/
class Camera : public Widget {
    template <typename Self, typename Finished>
    friend class builder::BuilderMixin;

    using Widget::Widget;

public:
    struct Tag {
        float corners[4][2];
        uint8_t id;
    };

    void setRotation(float rotation) {
        m_state->set("rotation", new rbjson::Number(rotation));
    }

    float rotation() const {
        return data().getDouble("rotation");
    }

    void setClip(bool clip) {
        m_state->set("clip", new rbjson::Bool(clip));
    }

    bool clip() const {
        return data().getBool("clip");
    }

    void addTag(const Tag& t) {
        auto *tagsArray = data().getArray("tags");
        const bool existedBefore = tagsArray != NULL;
        if(!existedBefore) {
            tagsArray = new rbjson::Array;
        }
        tagsArray->push_back(buildTagObject(t));

        if(!existedBefore) {
            m_state->set("tags", tagsArray);
        } {
            m_state->markChanged("tags");
        }
    }

    void setTags(const std::vector<Tag>& tags) {
        auto *tagsArray = new rbjson::Array;
        for(const auto& t : tags)  {
            tagsArray->push_back(buildTagObject(t));
        }

        m_state->set("tags", tagsArray);
    }

    void clearTags() {
        auto *tagsArray = new rbjson::Array;
        m_state->set("tags", tagsArray);
    }

private:
    rbjson::Object *buildTagObject(const Tag& t) {
        auto *tag = new rbjson::Object;
        tag->set("id", t.id);
        tag->set("c00", t.corners[0][0]);
        tag->set("c01", t.corners[0][1]);
        tag->set("c10", t.corners[1][0]);
        tag->set("c11", t.corners[1][1]);
        tag->set("c20", t.corners[2][0]);
        tag->set("c21", t.corners[2][1]);
        tag->set("c30", t.corners[3][0]);
        tag->set("c31", t.corners[3][1]);
        return tag;
    }
};

};
