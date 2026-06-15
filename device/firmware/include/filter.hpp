#pragma once

#include "sensor_reading.hpp"
#include <cstdint>

class ReadingFilter {
public:
    ReadingFilter(uint32_t interval_ms = 120000);

    void add_sample(const SensorReading& reading);
    bool check_and_get_reading(SensorReading& out_reading, uint32_t current_time_ms);

private:
    uint32_t m_interval_ms;
    uint32_t m_last_published_time_ms;
    bool m_has_last_published;

    // Accumulators for averaging
    double m_sum_temp;
    double m_sum_humidity;
    double m_sum_pressure;
    int m_sample_count;
};
