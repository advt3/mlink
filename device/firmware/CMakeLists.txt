cmake_minimum_required(VERSION 3.20.0)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/local.conf")
    set(EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/local.conf" CACHE INTERNAL "Local configuration overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device_firmware)

message(STATUS "DEBUG: EXTRA_CONF_FILE='${EXTRA_CONF_FILE}'")

zephyr_include_directories(include)

# Common sources
target_sources(app PRIVATE src/main.cpp src/runner.cpp src/filter.cpp)

# Network Configuration
if(CONFIG_WIFI)
    message(STATUS "Configuring for Wi-Fi Connection")
    target_sources(app PRIVATE src/network_connection_wifi.cpp)
else()
    message(STATUS "No Wi-Fi Configured. Configuring Mock Network Connection")
    target_sources(app PRIVATE src/network_connection_noop.cpp)
endif()

# Telemetry Configuration
if(CONFIG_APP_TELEMETRY_MQTT)
    message(STATUS "Configuring for MQTT Telemetry")
    target_sources(app PRIVATE src/mqtt_publisher.cpp)
endif()

# Build-time hardware selection
dt_alias(has_strip PROPERTY "led-strip")
dt_alias(has_led   PROPERTY "led0")
dt_nodelabel(has_bme280 NODELABEL "bme280")

message(STATUS "DEBUG: has_strip='${has_strip}'")
message(STATUS "DEBUG: has_led='${has_led}'")
message(STATUS "DEBUG: has_bme280='${has_bme280}'")

# Board options
if(has_strip)
    message(STATUS "Configuring for LED Strip")
    target_sources(app PRIVATE src/status_light_strip.cpp)
elseif(has_led)
    message(STATUS "Configuring for GPIO LED")
    target_sources(app PRIVATE src/status_light_gpio.cpp)
else()
    message(STATUS "No LED hardware found. Configuring Mock Status Light")
    target_sources(app PRIVATE src/status_light_noop.cpp)
endif()

if(has_bme280)
    message(STATUS "Configuring for BME280 Sensor")
    target_sources(app PRIVATE src/environment_sensor_bme280.cpp)
else()
    message(STATUS "No sensor hardware found. Configuring Mock Sensor")
    target_sources(app PRIVATE src/environment_sensor_noop.cpp)
endif()
