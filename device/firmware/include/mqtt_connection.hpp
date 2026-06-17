#pragma once
#include <zephyr/net/mqtt.h>
#include <zephyr/kernel.h>

class MqttConnection {
public:
    MqttConnection(const char *client_id);
    ~MqttConnection();
    
    bool initialize();
    bool start();
    void stop();
    bool get_connected();
    void set_connected(bool connected);
    struct mqtt_client* get_client();
    struct k_mutex* get_mutex();

private:
    struct mqtt_client client_;
    struct sockaddr_in broker_addr_;
    uint8_t rx_buffer_[256];
    uint8_t tx_buffer_[256];
    char mac_str_[18];
    bool is_connected_;
    bool should_run_;
    struct k_mutex state_mutex_;

    void run_loop();
    void process_network(int timeout_ms);
    bool connect_broker();
    void disconnect_broker();

    static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);
};
