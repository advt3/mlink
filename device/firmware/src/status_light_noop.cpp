#include "status_light.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(blinky, LOG_LEVEL_INF);

bool StatusLight::initialize() {
    LOG_INF("No status light hardware configured. Using mock status light.");
    m_initialized = true;
    return true;
}

void StatusLight::set_color(Color c) {
    // No-op
}
