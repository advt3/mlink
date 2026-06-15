#include "runner.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(blinky, LOG_LEVEL_INF);

Runner::Runner()
    : m_state(State::RUNNING), m_last_sample_time_ms(0),
      m_last_blink_toggle_time_ms(0), m_measure_blink_active(false),
      m_measure_blink_start_ms(0), m_failure_led_state(false) {}

bool Runner::initialize() {
  m_state = State::RUNNING;

  if (!m_status_light.initialize()) {
    LOG_ERR("Failed to initialize status light");
    m_state = State::FAILURE;
  }

  if (!m_sensor.initialize()) {
    LOG_ERR("Failed to initialize environment sensor");
    m_state = State::FAILURE;
  }

  uint32_t now_ms = k_uptime_get_32();
  // Set last sample time to trigger an immediate sample on startup
  m_last_sample_time_ms = now_ms - 30000;
  m_last_blink_toggle_time_ms = now_ms;

  if (m_state == State::FAILURE) {
    m_failure_led_state = true;
    m_status_light.set_color(StatusLight::Color{0x1F, 0, 0});
  }

  return true;
}

void Runner::run() {
  LOG_INF("Runner loop started");

  while (true) {
    uint32_t now_ms = k_uptime_get_32();

    // 1. Sensor Task: Sample every 30 seconds
    if (now_ms - m_last_sample_time_ms >= 3000) {
      m_last_sample_time_ms = now_ms;

      SensorReading reading;
      if (m_sensor.read(reading)) {
        m_filter.add_sample(reading);

        if (m_state == State::FAILURE) {
          LOG_INF("System recovered from failure state");
          m_state = State::RUNNING;
          m_status_light.set_color(StatusLight::Color::Black());
        }

        SensorReading avg_reading;
        if (m_filter.check_and_get_reading(avg_reading, now_ms)) {
          LOG_INF("Averaged Reading (2m): Temp = %.2f C, Hum = %.2f %%, Press "
                  "= %.2f hPa",
                  avg_reading.temperature, avg_reading.humidity,
                  avg_reading.pressure);

          m_measure_blink_active = true;
          m_measure_blink_start_ms = now_ms;
          m_status_light.set_color(StatusLight::Color::Green());
        }
      } else {
        if (m_state != State::FAILURE) {
          LOG_ERR("Sensor read failure! Entering failure state");
          m_state = State::FAILURE;
          m_last_blink_toggle_time_ms = now_ms;
          m_failure_led_state = true;
          m_status_light.set_color(StatusLight::Color{0x1F, 0, 0});
        }
      }
    }

    // 2. LED Signaling Task
    update_led_state(now_ms);

    k_msleep(10);
  }
}

void Runner::update_led_state(uint32_t now_ms) {
  if (m_state == State::FAILURE) {
    // Toggle LED every 500ms
    if (now_ms - m_last_blink_toggle_time_ms >= 500) {
      m_failure_led_state = !m_failure_led_state;
      m_last_blink_toggle_time_ms = now_ms;

      if (m_failure_led_state) {
        m_status_light.set_color(StatusLight::Color{0x1F, 0, 0});
      } else {
        m_status_light.set_color(StatusLight::Color::Black());
      }
    }
  } else {
    // Normal RUNNING state
    if (m_measure_blink_active) {
      if (now_ms - m_measure_blink_start_ms >= 2000) {
        m_measure_blink_active = false;
        m_status_light.set_color(StatusLight::Color::Black());
      }
    }
  }
}
