#include "network_connection.hpp"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

LOG_MODULE_REGISTER(network_connection, LOG_LEVEL_INF);

// Background thread configuration
#define CONNECTION_THREAD_STACK_SIZE 2048
#define CONNECTION_THREAD_PRIORITY 7

static K_THREAD_STACK_DEFINE(conn_thread_stack, CONNECTION_THREAD_STACK_SIZE);
static struct k_thread conn_thread_data;

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static bool g_wifi_connected = false;
static bool g_ipv4_assigned = false;
static bool g_should_run = true;

static void wifi_mgmt_handler(struct net_mgmt_event_callback *cb,
                              uint64_t mgmt_event, struct net_if *iface) {
  if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
    const struct wifi_status *status = (const struct wifi_status *)cb->info;
    if (status->status) {
      LOG_ERR("WiFi Connection failed (%d)", status->status);
      g_wifi_connected = false;
    } else {
      LOG_INF("WiFi WLAN connected");
      g_wifi_connected = true;
    }
  } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
    LOG_INF("WiFi WLAN disconnected");
    g_wifi_connected = false;
    g_ipv4_assigned = false;
  }
}

static void ipv4_mgmt_handler(struct net_mgmt_event_callback *cb,
                              uint64_t mgmt_event, struct net_if *iface) {
  if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
    LOG_INF("IPv4 address added successfully");
    g_ipv4_assigned = true;
  } else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL) {
    LOG_INF("IPv4 address removed");
    g_ipv4_assigned = false;
  }
}

static void connection_thread_entry(void *p1, void *p2, void *p3) {
  struct net_if *iface = net_if_get_default();
  if (!iface) {
    LOG_ERR("No default network interface found");
    return;
  }

  // Register management callbacks
  net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_handler,
                               NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
  net_mgmt_add_event_callback(&wifi_cb);

  net_mgmt_init_event_callback(&ipv4_cb, ipv4_mgmt_handler,
                               NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
  net_mgmt_add_event_callback(&ipv4_cb);

  struct wifi_connect_req_params params = {0};
  params.ssid = (const uint8_t *)CONFIG_APP_WIFI_SSID;
  params.ssid_length = strlen(CONFIG_APP_WIFI_SSID);
  params.psk = (const uint8_t *)CONFIG_APP_WIFI_PASSWORD;
  params.psk_length = strlen(CONFIG_APP_WIFI_PASSWORD);
  params.channel = WIFI_CHANNEL_ANY;
  params.security = WIFI_SECURITY_TYPE_PSK;

  LOG_INF("Connection manager thread started. Targeting SSID: %s", CONFIG_APP_WIFI_SSID);

  while (g_should_run) {
    if (!g_wifi_connected || !g_ipv4_assigned) {
      LOG_INF("WiFi or IP connection not established. Attempting connection...");
      
      int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
      if (ret) {
        LOG_ERR("WiFi connection request failed immediately (ret=%d)", ret);
      }
      
      // Wait before checking or attempting again to prevent log flooding
      k_sleep(K_SECONDS(15));
    } else {
      // Periodic check interval when healthy
      k_sleep(K_SECONDS(5));
    }
  }

  net_mgmt_del_event_callback(&wifi_cb);
  net_mgmt_del_event_callback(&ipv4_cb);
}

NetworkConnection::NetworkConnection() : m_initialized(false) {}

bool NetworkConnection::initialize() {
  if (m_initialized) {
    return true;
  }

  k_thread_create(&conn_thread_data, conn_thread_stack,
                  K_THREAD_STACK_SIZEOF(conn_thread_stack),
                  connection_thread_entry,
                  NULL, NULL, NULL,
                  CONNECTION_THREAD_PRIORITY, 0, K_NO_WAIT);

  m_initialized = true;
  return true;
}

bool NetworkConnection::is_connected() const {
  return m_initialized && g_wifi_connected && g_ipv4_assigned;
}
