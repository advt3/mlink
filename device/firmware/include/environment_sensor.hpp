#pragma once

#include "sensor_reading.hpp"

class EnvironmentSensor {
public:
    EnvironmentSensor();
    bool initialize();
    bool read(SensorReading& out);

private:
    bool m_initialized = false;
};
