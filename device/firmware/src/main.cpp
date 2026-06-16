#include "runner.hpp"
#include "telemetry_publisher.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mlink, LOG_LEVEL_INF);

int main() {
  // Allow USB console to connect on the host PC after reset
  k_msleep(2000);

  LOG_INF("Starting environment metrics firmware runner...");

  TelemetryPublisher publisher;
  Runner runner(publisher);
  if (!runner.initialize()) {
    LOG_ERR("Failed to initialize firmware runner");
    return -1;
  }

  if (!publisher.start()) {
    LOG_ERR("Failed to start telemetry publisher");
    return -1;
  }

  runner.run();

  publisher.stop();
  return 0;
}
