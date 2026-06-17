#include "runner.hpp"
#include "telemetry_publisher.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <stdio.h>

LOG_MODULE_REGISTER(mlink, LOG_LEVEL_INF);

int main() {
  // Allow USB console to connect on the host PC after reset
  k_msleep(2000);

  LOG_INF("Starting environment metrics firmware runner...");

  char client_id[13] = "000000000000";
  struct net_if *iface = net_if_get_default();
  if (iface) {
    struct net_linkaddr *link_addr = net_if_get_link_addr(iface);
    if (link_addr && link_addr->len == 6) {
      snprintf(client_id, sizeof(client_id), "%02X%02X%02X%02X%02X%02X",
               link_addr->addr[0], link_addr->addr[1], link_addr->addr[2],
               link_addr->addr[3], link_addr->addr[4], link_addr->addr[5]);
    }
  }

  TelemetryPublisher publisher(client_id);
  Runner runner(publisher);
  if (!runner.initialize()) {
    LOG_ERR("Failed to initialize firmware runner");
    return -1;
  }

  runner.run();

  return 0;
}
