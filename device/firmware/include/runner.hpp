#pragma once

#include "status_light.hpp"
#include "environment_sensor.hpp"
#include "filter.hpp"
#include "network_connection.hpp"
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
    NetworkConnection m_network;
    State m_state;

    // Task intervals and last run timestamps
    uint32_t m_last_sample_time_ms;
    uint32_t m_last_blink_toggle_time_ms;

    // Measurement blink control
    bool m_measure_blink_active;
    uint32_t m_measure_blink_start_ms;

    // Failure blink state
    bool m_failure_led_state;

    struct Events {
        bool network_ok;
        bool sensor_sampled;
        bool sensor_ok;
    };

    void update_led_state(uint32_t now_ms);
    void transition_to(State new_state, uint32_t now_ms, const char* reason);
    void process_reading(uint32_t now_ms);
    
    Events gather_events(uint32_t now_ms);
    void evaluate_state(uint32_t now_ms, const Events& events);
};
