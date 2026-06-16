#pragma once
#include "sensor_reading.hpp"

class NoOpPublisher {
public:
    NoOpPublisher() = default;
    bool start() { return true; }
    void stop() {}
    bool publish(const SensorReading&) { return true; }
    bool get_connected() { return true; }
};
