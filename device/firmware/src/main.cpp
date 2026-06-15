#include "runner.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(blinky, LOG_LEVEL_INF);

int main() {
  // Allow USB console to connect on the host PC after reset
  k_msleep(2000);

  LOG_INF("Starting environment metrics firmware runner...");

  Runner runner;
  if (!runner.initialize()) {
    LOG_ERR("Failed to initialize firmware runner");
    return -1;
  }

  runner.run();

  return 0;
}
