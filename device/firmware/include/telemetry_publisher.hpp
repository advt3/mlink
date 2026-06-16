#pragma once

#if defined(CONFIG_APP_TELEMETRY_MQTT)
#include "mqtt_publisher.hpp"
using TelemetryPublisher = MqttPublisher;
#else
#include "noop_publisher.hpp"
using TelemetryPublisher = NoOpPublisher;
#endif
