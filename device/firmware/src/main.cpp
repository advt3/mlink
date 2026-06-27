#include "runner.hpp"
#include "telemetry_publisher.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/sntp.h>
#include <time.h>
#include <stdio.h>

LOG_MODULE_REGISTER(mlink, LOG_LEVEL_INF);

static volatile uint32_t s_boot_time_seconds = 0;

static log_timestamp_t real_time_logger_timestamp(void) {
    uint32_t boot_sec = s_boot_time_seconds;
    if (boot_sec == 0) {
        return (log_timestamp_t)(k_uptime_get_32() / 1000);
    }
    return (log_timestamp_t)((k_uptime_get_32() / 1000) + boot_sec);
}

static void sync_time() {
    struct sntp_time sntp_time_data;
    LOG_INF("Attempting to sync time via SNTP...");
    
    int ret = sntp_simple("pool.ntp.org", 5000, &sntp_time_data);
    if (ret == 0) {
        struct timespec ts;
        ts.tv_sec = sntp_time_data.seconds;
        ts.tv_nsec = 0;
        if (clock_settime(CLOCK_REALTIME, &ts) == 0) {
            struct tm broken_time;
            gmtime_r(&ts.tv_sec, &broken_time);
            LOG_INF("Time synchronized successfully. Current year: %d", broken_time.tm_year + 1900);
        } else {
            LOG_ERR("Failed to set system clock");
        }
    } else {
        LOG_ERR("SNTP query failed: %d", ret);
    }
}

int main() {
  // Register custom 1Hz real-time log timestamp provider
  log_set_timestamp_func(real_time_logger_timestamp, 1);

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

  LOG_INF("Device MAC: %s", client_id);

  TelemetryPublisher publisher(client_id);
  Runner runner(publisher);
  if (!runner.initialize()) {
    LOG_ERR("Failed to initialize firmware runner");
    return -1;
  }

  LOG_INF("Waiting for WiFi and IP connection...");
  while (!runner.is_network_connected()) {
      k_msleep(1000);
  }
  LOG_INF("Network connected!");

  // Synchronize time (retry until success)
  while (true) {
      sync_time();
      time_t now = time(NULL);
      if (now >= 1700000000) {
          s_boot_time_seconds = (uint32_t)now - (k_uptime_get_32() / 1000);
          break;
      }
      LOG_WRN("Time sync not verified (current time: %lld). Retrying in 5 seconds...", (long long)now);
      k_msleep(5000);
  }

  runner.run();

  return 0;
}
