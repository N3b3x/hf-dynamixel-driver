/**
 * @file dynamixel.hpp
 * @brief Umbrella header for the HF Dynamixel Protocol 2.0 driver.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include "dynamixel_types.hpp"
#include "dynamixel_transport.hpp"
#include "dynamixel_model.hpp"
#include "dynamixel_bus.hpp"
#include "dynamixel_device.hpp"
#include "dynamixel_version.h"

namespace dynamixel {

inline const char* GetDriverVersion() noexcept {
    return HF_DYNAMIXEL_VERSION_STRING;
}

inline const char* GetSdkCommit() noexcept {
    return HF_DYNAMIXEL_SDK_COMMIT;
}

inline const char* GetSdkTag() noexcept {
    return HF_DYNAMIXEL_SDK_TAG;
}

}  // namespace dynamixel
