#include "environment_sensor.hpp"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(blinky, LOG_LEVEL_INF);

#define SENSOR_NODE DT_NODELABEL(bme280)

namespace {
const struct device *const dev = DEVICE_DT_GET(SENSOR_NODE);
}

EnvironmentSensor::EnvironmentSensor() : m_initialized(false) {}

bool EnvironmentSensor::initialize() {
  if (!device_is_ready(dev)) {
    LOG_ERR("BME280 sensor device is not ready");
    return false;
  }
  LOG_INF("Found BME280 sensor device: %s", dev->name);
  m_initialized = true;
  return true;
}

bool EnvironmentSensor::read(SensorReading &out) {
  if (!m_initialized) {
    LOG_ERR("BME280 sensor not initialized");
    return false;
  }

  struct sensor_value temp, press, humidity;

  if (sensor_sample_fetch(dev) < 0) {
    LOG_ERR("BME280 sample fetch failed");
    return false;
  }

  sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
  sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
  sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

  out.temperature = sensor_value_to_double(&temp);
  out.pressure = sensor_value_to_double(&press) * 10.0; // kPa to hPa
  out.humidity = sensor_value_to_double(&humidity);

#if DT_NODE_HAS_PROP(SENSOR_NODE, reg)
  out.address = DT_REG_ADDR(SENSOR_NODE);
#else
  out.address = 0;
#endif

  return true;
}
