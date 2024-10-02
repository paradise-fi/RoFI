#pragma once

#include <nlohmann/json.hpp>
#include "compute.hpp"

namespace rofi::torqueComputation {
    /**
     * Transforms config to JSON representation.
     * @param config
     * @return
     */
    nlohmann::json toJSON(const rofi::torqueComputation::TorqueConfig& config);

    /**
     * Creates config from provided JSON.
     * @param json
     * @return
     */
    rofi::torqueComputation::TorqueConfig fromJSON(const nlohmann::json& json);
}
