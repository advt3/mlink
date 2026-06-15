#include "status_light.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(blinky, LOG_LEVEL_INF);

/* 1000 msec = 1 sec */
constexpr k_timeout_t sleep_time = K_MSEC(1500);

int main() {
  StatusLight status_light;

  if (!status_light.initialize()) {
    LOG_ERR("Failed to initialize status light");
    return -1;
  }

  LOG_INF("Blinky started successfully");

  bool status_light_state = false;
  while (true) {
    status_light_state = !status_light_state;
    status_light.set_state(status_light_state);
    k_sleep(sleep_time);
    LOG_INF("Status light state: %s", status_light_state ? "ON" : "OFF");
  }
  return 0;
}
