#include "status_light.hpp"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(mlink, LOG_LEVEL_INF);

#define STRIP_NODE DT_ALIAS(led_strip)

namespace {
    const struct device *const dev = DEVICE_DT_GET(STRIP_NODE);
    constexpr size_t count = DT_PROP_OR(STRIP_NODE, chain_length, 1);
    struct led_rgb pixels[count];
}

bool StatusLight::initialize() {
    if (!device_is_ready(dev)) return false;
    m_initialized = true;
    set_color(Color::Black());
    return true;
}

void StatusLight::set_color(Color c) {
    if (!m_initialized) return;
    for (auto &p : pixels) p = {c.r, c.g, c.b};
    led_strip_update_rgb(dev, pixels, count);
}
