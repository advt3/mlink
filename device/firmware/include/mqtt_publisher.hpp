#pragma once
#include "sensor_reading.hpp"
#include "mqtt_connection.hpp"

class MqttPublisher {
public:
    MqttPublisher(const char *client_id);
    ~MqttPublisher() = default;
    
    bool initialize();
    bool start();
    void stop();
    bool publish(const SensorReading& reading);
    bool get_connected();

private:
    MqttConnection connection_;
};
