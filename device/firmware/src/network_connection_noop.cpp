#include "network_connection.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(network_connection, LOG_LEVEL_INF);

NetworkConnection::NetworkConnection() : m_initialized(false) {}

bool NetworkConnection::initialize() {
    LOG_INF("Initializing Mock Network Connection (No-op)");
    m_initialized = true;
    return true;
}

bool NetworkConnection::is_connected() const {
    return m_initialized;
}
