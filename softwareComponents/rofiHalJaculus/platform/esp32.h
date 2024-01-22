#pragma once

#include <jac/machine/machine.h>
#include <set>
#include <unordered_map>
#include <string>

#include "hal/adc_types.h"
#include "soc/adc_channel.h"


template<class Next>
class PlatformInfoFeature : public Next {
public:
    struct PlatformInfo {
        static inline const std::string NAME = "ESP32";
    };

    void initialize() {
        Next::initialize();

        jac::ContextRef ctx = this->context();

        jac::Object platformInfo = jac::Object::create(ctx);
        platformInfo.defineProperty("name", jac::Value::from(ctx, PlatformInfo::NAME), jac::PropFlags::Enumerable);

        ctx.getGlobalObject().defineProperty("PlatformInfo", platformInfo, jac::PropFlags::Enumerable);
    }
};
