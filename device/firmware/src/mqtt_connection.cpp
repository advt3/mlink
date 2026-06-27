#include "mqtt_connection.hpp"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

LOG_MODULE_DECLARE(mlink, LOG_LEVEL_INF);

#if defined(CONFIG_MQTT_LIB_TLS)
#include <zephyr/net/tls_credentials.h>

#define MQTT_TLS_SEC_TAG 1

static const char ca_certificate[] =
#include "ca_cert.inc"
    ;

static int register_credentials(void) {
  int rc = tls_credential_add(MQTT_TLS_SEC_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
                              ca_certificate, sizeof(ca_certificate));
  if (rc < 0 && rc != -EEXIST) {
    LOG_ERR("Failed to register CA certificate: %d", rc);
    return rc;
  }
  return 0;
}
#endif

// Define a dedicated thread space for the MQTT network runner
K_THREAD_STACK_DEFINE(mqtt_worker_stack, 8192);
static struct k_thread mqtt_worker_data;


void MqttConnection::mqtt_evt_handler(struct mqtt_client *const client,
                                      const struct mqtt_evt *evt) {
  MqttConnection *conn = static_cast<MqttConnection *>(client->user_data);
  if (!conn)
    return;

  switch (evt->type) {
  case MQTT_EVT_CONNACK:
    if (evt->param.connack.return_code != 0) {
      LOG_ERR("MQTT connection refused: %d", evt->param.connack.return_code);
      conn->set_connected(false);
      } else {
      LOG_INF("MQTT client successfully connected!");
      conn->set_connected(true);
      }
    break;

  case MQTT_EVT_DISCONNECT:
    LOG_WRN("MQTT client disconnected: %d", evt->result);
    conn->set_connected(false);
    break;

  case MQTT_EVT_PUBACK:
    if (evt->result != 0) {
      LOG_ERR("MQTT PUBACK error: %d", evt->result);
    } else {
      LOG_DBG("PUBACK received for packet id: %u",
              evt->param.puback.message_id);
    }
    break;

  default:
    break;
  }
}

MqttConnection::MqttConnection(const char *client_id)
    : is_connected_(false), should_run_(false) {
  k_mutex_init(&state_mutex_);
  if (client_id) {
    strncpy(mac_str_, client_id, sizeof(mac_str_) - 1);
    mac_str_[sizeof(mac_str_) - 1] = '\0';
  } else {
    strcpy(mac_str_, "000000000000");
  }
}

MqttConnection::~MqttConnection() { stop(); }

bool MqttConnection::initialize() {
  mqtt_client_init(&client_);

  memset(&broker_addr_, 0, sizeof(broker_addr_));
  broker_addr_.sin_family = AF_INET;
  broker_addr_.sin_port = htons(CONFIG_APP_MQTT_BROKER_PORT);

  client_.broker = &broker_addr_;
  client_.evt_cb = mqtt_evt_handler;
  client_.user_data = this;
  client_.client_id.utf8 = (const uint8_t *)mac_str_;
  client_.client_id.size = strlen(mac_str_);

  static struct mqtt_utf8 username_utf8;
  static struct mqtt_utf8 password_utf8;

  username_utf8.utf8 = (const uint8_t *)CONFIG_APP_MQTT_USERNAME;
  username_utf8.size = strlen(CONFIG_APP_MQTT_USERNAME);

  password_utf8.utf8 = (const uint8_t *)CONFIG_APP_MQTT_PASSWORD;
  password_utf8.size = strlen(CONFIG_APP_MQTT_PASSWORD);

  client_.user_name = &username_utf8;
  client_.password = &password_utf8;

  client_.protocol_version = MQTT_VERSION_3_1_1;
  client_.keepalive = 60;

  client_.rx_buf = rx_buffer_;
  client_.rx_buf_size = sizeof(rx_buffer_);
  client_.tx_buf = tx_buffer_;
  client_.tx_buf_size = sizeof(tx_buffer_);

#if defined(CONFIG_MQTT_LIB_TLS)
  int rc = register_credentials();
  if (rc < 0) {
    return false;
  }

  static sec_tag_t sec_tag_list[] = {MQTT_TLS_SEC_TAG};
  client_.transport.type = MQTT_TRANSPORT_SECURE;
  struct mqtt_sec_config *tls_config = &client_.transport.tls.config;

#if defined(CONFIG_APP_MQTT_TLS_VERIFY_REQUIRED)
  tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
#else
  tls_config->peer_verify = TLS_PEER_VERIFY_NONE;
#endif

  tls_config->cipher_list = NULL;
  tls_config->cipher_count = 0;
  tls_config->sec_tag_list = sec_tag_list;
  tls_config->sec_tag_count = ARRAY_SIZE(sec_tag_list);
  tls_config->hostname = CONFIG_APP_MQTT_BROKER_HOST;
#else
  client_.transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif

  return true;
}

bool MqttConnection::start() {
  if (should_run_)
    return true;

  should_run_ = true;

  k_thread_create(
      &mqtt_worker_data, mqtt_worker_stack,
      K_THREAD_STACK_SIZEOF(mqtt_worker_stack),
      [](void *p1, void *, void *) {
        static_cast<MqttConnection *>(p1)->run_loop();
      },
      this, nullptr, nullptr, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

  return true;
}

void MqttConnection::stop() {
  should_run_ = false;
  disconnect_broker();
  k_sleep(K_MSEC(200));
}

bool MqttConnection::connect_broker() {
#if defined(CONFIG_DNS_RESOLVER)
  struct zsock_addrinfo hints;
  struct zsock_addrinfo *res = nullptr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  LOG_INF("Resolving broker hostname: %s", CONFIG_APP_MQTT_BROKER_HOST);
  int err = zsock_getaddrinfo(CONFIG_APP_MQTT_BROKER_HOST, NULL, &hints, &res);
  if (err == 0 && res != nullptr) {
    struct sockaddr_in *resolved_addr = (struct sockaddr_in *)res->ai_addr;
    broker_addr_.sin_addr = resolved_addr->sin_addr;
    char ip_str[INET_ADDRSTRLEN];
    zsock_inet_ntop(AF_INET, &broker_addr_.sin_addr, ip_str, sizeof(ip_str));
    LOG_INF("Resolved %s to %s", CONFIG_APP_MQTT_BROKER_HOST, ip_str);
    zsock_freeaddrinfo(res);
  } else {
    LOG_WRN("DNS resolution failed (%d), falling back to direct IP parsing",
            err);
    if (zsock_inet_pton(AF_INET, CONFIG_APP_MQTT_BROKER_HOST,
                        &broker_addr_.sin_addr) != 1) {
      LOG_ERR("Failed to parse broker address: %s",
              CONFIG_APP_MQTT_BROKER_HOST);
      return false;
    }
  }
#else
  if (zsock_inet_pton(AF_INET, CONFIG_APP_MQTT_BROKER_HOST,
                      &broker_addr_.sin_addr) != 1) {
    LOG_ERR("Failed to parse broker address: %s", CONFIG_APP_MQTT_BROKER_HOST);
    return false;
  }
#endif

  int rc = mqtt_connect(&client_);
  if (rc != 0) {
    LOG_ERR("mqtt_connect failed: %d", rc);
    return false;
  }
  return true;
}

void MqttConnection::disconnect_broker() {
  mqtt_disconnect(&client_, NULL);

  int sock = -1;
#if defined(CONFIG_MQTT_LIB_TLS)
  sock = client_.transport.tls.sock;
#else
  sock = client_.transport.tcp.sock;
#endif

  if (sock >= 0) {
    zsock_close(sock);
#if defined(CONFIG_MQTT_LIB_TLS)
    client_.transport.tls.sock = -1;
#else
    client_.transport.tcp.sock = -1;
#endif
    LOG_INF("MQTT socket explicitly closed to release resources.");
  }

  set_connected(false);
}

static bool is_network_ready(void) {
  struct net_if *iface = net_if_get_default();
  if (!iface) {
    return false;
  }

#if defined(CONFIG_NET_IPV4)
  struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
  if (!ipv4) {
    return false;
  }

  for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
    if (ipv4->unicast[i].ipv4.is_used &&
        ipv4->unicast[i].ipv4.addr_state == NET_ADDR_PREFERRED) {
      return true;
    }
  }
#endif

  return false;
}

void MqttConnection::run_loop() {
  enum class State {
    INIT,
    WAIT_NETWORK,
    CONNECTING,
    WAIT_CONNACK,
    CONNECTED,
    ERROR_DELAY
  };

  auto state_name = [](State s) -> const char * {
    switch (s) {
    case State::INIT:
      return "INIT";
    case State::WAIT_NETWORK:
      return "WAIT_NETWORK";
    case State::CONNECTING:
      return "CONNECTING";
    case State::WAIT_CONNACK:
      return "WAIT_CONNACK";
    case State::CONNECTED:
      return "CONNECTED";
    case State::ERROR_DELAY:
      return "ERROR_DELAY";
    }
    return "UNKNOWN";
  };

  State state = State::INIT;
  State last_state = State::ERROR_DELAY; // Force log on initial loop
  uint32_t state_timer_ms = 0;

  while (should_run_) {
    uint32_t now = k_uptime_get_32();

    if (state != last_state) {
      LOG_INF("FSM State: %s", state_name(state));
      last_state = state;
    }

    switch (state) {
    case State::INIT:
      disconnect_broker();
      state = State::WAIT_NETWORK;
      break;

    case State::WAIT_NETWORK:
      if (is_network_ready()) {
        state = State::CONNECTING;
      } else {
        k_sleep(K_MSEC(1000));
      }
      break;

    case State::CONNECTING:
      LOG_INF("Attempting connection to MQTT broker...");
      if (connect_broker()) {
        state = State::WAIT_CONNACK;
        state_timer_ms = k_uptime_get_32();
      } else {
        LOG_WRN("Connection establishment failed (TCP/TLS handshake error). "
                "Retrying in 5 seconds...");
        state = State::ERROR_DELAY;
        state_timer_ms = k_uptime_get_32();
      }
      break;

    case State::WAIT_CONNACK:
      process_network(100);
      if (get_connected()) {
        state = State::CONNECTED;
      } else if (now - state_timer_ms >= 30000) {
        LOG_WRN("Timed out waiting for CONNACK from broker. Retrying in 5 "
                "seconds...");
        state = State::ERROR_DELAY;
        state_timer_ms = now;
      }
      break;

    case State::CONNECTED:
      process_network(500);
      if (!get_connected()) {
        LOG_WRN("Connection lost. Retrying in 5 seconds...");
        state = State::ERROR_DELAY;
        state_timer_ms = now;
      }
      break;

    case State::ERROR_DELAY:
      disconnect_broker();
      if (now - state_timer_ms >= 5000) {
        state = State::WAIT_NETWORK;
      } else {
        k_sleep(K_MSEC(500));
      }
      break;
    }
  }
}

void MqttConnection::process_network(int timeout_ms) {
  struct zsock_pollfd fds[1];
  int sock = -1;

#if defined(CONFIG_MQTT_LIB_TLS)
  sock = client_.transport.tls.sock;
#else
  sock = client_.transport.tcp.sock;
#endif

  if (sock >= 0) {
    fds[0].fd = sock;
    fds[0].events = ZSOCK_POLLIN;

    int rc = zsock_poll(fds, 1, timeout_ms);
    if (rc > 0) {
      LOG_DBG("zsock_poll rc=%d, revents=0x%x", rc, fds[0].revents);
      if (fds[0].revents & ZSOCK_POLLIN) {
        rc = mqtt_input(&client_);
        LOG_DBG("mqtt_input rc=%d", rc);
        if (rc < 0) {
          LOG_ERR("mqtt_input error: %d", rc);
          set_connected(false);
        }
      }
      if (fds[0].revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP | ZSOCK_POLLNVAL)) {
        LOG_ERR("Socket error, hangup, or invalid fd detected via poll: 0x%x",
                fds[0].revents);
        set_connected(false);
      }
    } else if (rc < 0) {
      LOG_ERR("zsock_poll failed: %d", rc);
      set_connected(false);
    } else {
      // timeout (rc == 0)
      LOG_DBG("zsock_poll timeout (%d ms)", timeout_ms);
    }

    rc = mqtt_live(&client_);
    if (rc != 0 && rc != -EAGAIN) {
      LOG_ERR("mqtt_live error: %d", rc);
      set_connected(false);
    }
  } else {
    LOG_DBG("process_network: No socket active, sleeping %d ms", timeout_ms);
    k_sleep(K_MSEC(timeout_ms));
  }
}

void MqttConnection::set_connected(bool connected) {
  k_mutex_lock(&state_mutex_, K_FOREVER);
  is_connected_ = connected;
  k_mutex_unlock(&state_mutex_);
}

bool MqttConnection::get_connected() {
  k_mutex_lock(&state_mutex_, K_FOREVER);
  bool status = is_connected_;
  k_mutex_unlock(&state_mutex_);
  return status;
}

struct mqtt_client *MqttConnection::get_client() { return &client_; }

struct k_mutex *MqttConnection::get_mutex() { return &state_mutex_; }
