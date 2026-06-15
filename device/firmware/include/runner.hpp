#pragma once

#include "status_light.hpp"
#include "environment_sensor.hpp"
#include "filter.hpp"
#include <cstdint>

class Runner {
public:
    enum class State {
        RUNNING,
        FAILURE
    };

    Runner();
    bool initialize();
    void run();

private:
    StatusLight m_status_light;
    EnvironmentSensor m_sensor;
    ReadingFilter m_filter;
    State m_state;

    // Task intervals and last run timestamps
    uint32_t m_last_sample_time_ms;
    uint32_t m_last_blink_toggle_time_ms;

    // Measurement blink control
    bool m_measure_blink_active;
    uint32_t m_measure_blink_start_ms;

    // Failure blink state
    bool m_failure_led_state;

    void update_led_state(uint32_t now_ms);
};
