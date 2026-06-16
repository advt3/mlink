#include "runner.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(mlink, LOG_LEVEL_INF);

Runner::Runner(TelemetryPublisher& publisher)
    : m_publisher(publisher), m_state(State::RUNNING), m_network_established(false),
      m_last_sample_time_ms(0), m_last_blink_toggle_time_ms(0),
      m_measure_blink_active(false), m_measure_blink_start_ms(0),
      m_failure_led_state(false) {}

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

  if (!m_network.initialize()) {
    LOG_ERR("Failed to initialize network connection");
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

void Runner::transition_to(State new_state, uint32_t now_ms, const char* reason) {
  if (reason && reason[0] != '\0') {
    if (new_state == State::FAILURE) {
      LOG_ERR("%s", reason);
    } else {
      LOG_INF("%s", reason);
    }
  }

  m_state = new_state;

  if (m_state == State::FAILURE) {
    m_last_blink_toggle_time_ms = now_ms;
    m_failure_led_state = true;
    m_status_light.set_color(StatusLight::Color{0x1F, 0, 0});
  } else if (m_state == State::RUNNING) {
    m_status_light.set_color(StatusLight::Color::Black());
  }
}

void Runner::process_reading(uint32_t now_ms) {
  SensorReading avg_reading;
  if (m_filter.check_and_get_reading(avg_reading, now_ms)) {
    LOG_INF("Averaged Reading (2m): Temp = %.2f C, Hum = %.2f %%, Press "
            "= %.2f hPa [Network: %s]",
            avg_reading.temperature, avg_reading.humidity,
            avg_reading.pressure,
            m_network.is_connected() ? "CONNECTED" : "DISCONNECTED");

    m_measure_blink_active = true;
    m_measure_blink_start_ms = now_ms;
    m_status_light.set_color(StatusLight::Color::Green());

    if (m_network.is_connected()) {
      m_publisher.publish(avg_reading);
    }
  }
}

Runner::Events Runner::gather_events(uint32_t now_ms) {
  Events events = {
    .network_ok = m_network.is_connected() && m_publisher.get_connected(),
    .sensor_sampled = false,
    .sensor_ok = false
  };

  // Sample every 3 seconds
  if (now_ms - m_last_sample_time_ms >= 3000) {
    m_last_sample_time_ms = now_ms;
    events.sensor_sampled = true;

    SensorReading reading;
    events.sensor_ok = m_sensor.read(reading);
    if (events.sensor_ok) {
      m_filter.add_sample(reading);
    }
  }

  return events;
}

void Runner::evaluate_state(uint32_t now_ms, const Events& events) {
  if (events.network_ok) {
    m_network_established = true;
  }

  switch (m_state) {
    case State::RUNNING:
      if (!events.network_ok) {
        if (m_network_established) {
          transition_to(State::FAILURE, now_ms, "Network connection lost! Entering failure state");
        }
      } else if (events.sensor_sampled && !events.sensor_ok) {
        transition_to(State::FAILURE, now_ms, "Sensor read failure! Entering failure state");
      } else if (events.sensor_sampled && events.sensor_ok) {
        process_reading(now_ms);
      }
      break;

    case State::FAILURE:
      if (events.network_ok && events.sensor_sampled && events.sensor_ok) {
        transition_to(State::RUNNING, now_ms, "System recovered from failure state");
        process_reading(now_ms);
      }
      break;
  }
}

void Runner::run() {
  LOG_INF("Runner loop started");

  while (true) {
    uint32_t now_ms = k_uptime_get_32();

    Events events = gather_events(now_ms);
    evaluate_state(now_ms, events);
    update_led_state(now_ms);

    k_msleep(10);
  }
}

void Runner::update_led_state(uint32_t now_ms) {
  if (m_state == State::FAILURE) {
    // Toggle LED every 500ms (red blink for failure / disconnection)
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
    // Normal RUNNING state with active network connection
    if (m_measure_blink_active) {
      if (now_ms - m_measure_blink_start_ms >= 2000) {
        m_measure_blink_active = false;
        m_status_light.set_color(StatusLight::Color::Black());
      }
    }
  }
}
