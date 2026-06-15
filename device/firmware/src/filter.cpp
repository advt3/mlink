#include "filter.hpp"

ReadingFilter::ReadingFilter(uint32_t interval_ms)
    : m_interval_ms(interval_ms),
      m_last_published_time_ms(0),
      m_has_last_published(false),
      m_sum_temp(0.0),
      m_sum_humidity(0.0),
      m_sum_pressure(0.0),
      m_sample_count(0) {}

void ReadingFilter::add_sample(const SensorReading& reading) {
    m_sum_temp += reading.temperature;
    m_sum_humidity += reading.humidity;
    m_sum_pressure += reading.pressure;
    m_sample_count++;
}

bool ReadingFilter::check_and_get_reading(SensorReading& out_reading, uint32_t current_time_ms) {
    if (m_sample_count == 0) {
        return false;
    }

    if (!m_has_last_published) {
        out_reading.temperature = m_sum_temp / m_sample_count;
        out_reading.humidity = m_sum_humidity / m_sample_count;
        out_reading.pressure = m_sum_pressure / m_sample_count;
        out_reading.address = 0;

        m_last_published_time_ms = current_time_ms;
        m_has_last_published = true;

        m_sum_temp = 0.0;
        m_sum_humidity = 0.0;
        m_sum_pressure = 0.0;
        m_sample_count = 0;
        return true;
    }

    uint32_t elapsed = current_time_ms - m_last_published_time_ms;
    if (elapsed >= m_interval_ms) {
        out_reading.temperature = m_sum_temp / m_sample_count;
        out_reading.humidity = m_sum_humidity / m_sample_count;
        out_reading.pressure = m_sum_pressure / m_sample_count;
        out_reading.address = 0;

        m_last_published_time_ms = current_time_ms;

        m_sum_temp = 0.0;
        m_sum_humidity = 0.0;
        m_sum_pressure = 0.0;
        m_sample_count = 0;
        return true;
    }

    return false;
}
