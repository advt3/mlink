#include "status_light.hpp"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(mlink, LOG_LEVEL_INF);

#define LED_NODE DT_ALIAS(led0)

namespace {
    const struct gpio_dt_spec status_light = GPIO_DT_SPEC_GET(LED_NODE, gpios);
}

bool StatusLight::initialize() {
    if (!gpio_is_ready_dt(&status_light)) return false;
    if (gpio_pin_configure_dt(&status_light, GPIO_OUTPUT_INACTIVE) < 0) return false;
    m_initialized = true;
    return true;
}

void StatusLight::set_color(Color c) {
    if (!m_initialized) return;
    bool on = (c.r | c.g | c.b) > 0;
    gpio_pin_set_dt(&status_light, on ? 1 : 0);
}
