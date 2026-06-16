#include "environment_sensor.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(mlink, LOG_LEVEL_INF);

EnvironmentSensor::EnvironmentSensor() : m_initialized(false) {}

bool EnvironmentSensor::initialize() {
  LOG_INF(
      "No BME280 sensor hardware configured. Using mock environment sensor.");
  m_initialized = true;
  return true;
}

bool EnvironmentSensor::read(SensorReading &out) {
  if (!m_initialized) {
    LOG_ERR("Mock sensor not initialized");
    return false;
  }

  // Return mock environment values
  out.temperature = 23.5;
  out.humidity = 48.2;
  out.pressure = 1012.8;
  out.address = 0x00;

  return true;
}
