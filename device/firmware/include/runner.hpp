#pragma once

#include "status_light.hpp"
#include "environment_sensor.hpp"
#include "filter.hpp"
#include "network_connection.hpp"
#include "telemetry_publisher.hpp"
#include <cstdint>

#ifdef CONFIG_MENDER_MCU_CLIENT
#include "mender_client.hpp"
#endif


class Runner {
public:
    enum class State {
        RUNNING,
        FAILURE
    };

    Runner(TelemetryPublisher& publisher);
    bool initialize();
    void run();
    bool is_network_connected() const { return m_network.is_connected(); }


private:
    StatusLight m_status_light;
    EnvironmentSensor m_sensor;
    ReadingFilter m_filter;
    NetworkConnection m_network;
    TelemetryPublisher& m_publisher;
    State m_state;
    bool m_network_established;

#ifdef CONFIG_MENDER_MCU_CLIENT
    MenderClient m_mender_client;
#endif



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
