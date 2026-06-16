#pragma once
#include "sensor_reading.hpp"
#include <zephyr/net/mqtt.h>
#include <zephyr/kernel.h>

class MqttPublisher {
public:
    MqttPublisher();
    ~MqttPublisher();
    
    bool start();
    void stop();
    bool publish(const SensorReading& reading);
    void set_connected(bool connected);
    bool get_connected();

private:
    struct mqtt_client client_;
    struct sockaddr_in broker_addr_;
    uint8_t rx_buffer_[256];
    uint8_t tx_buffer_[256];
    char mac_str_[18];
    bool is_connected_;
    bool should_run_;
    struct k_mutex state_mutex_;

    void init_client();
    void run_loop();
    void process_network(int timeout_ms);
    bool connect_broker();
    void disconnect_broker();

    static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);
};
