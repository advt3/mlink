cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device_firmware)

zephyr_include_directories(include)

# Common sources
target_sources(app PRIVATE src/main.cpp)

# Build-time hardware selection
dt_alias(has_strip PROPERTY "led-strip")
dt_alias(has_led   PROPERTY "led0")

message(STATUS "DEBUG: has_strip='${has_strip}'")
message(STATUS "DEBUG: has_led='${has_led}'")

if(has_strip)
    message(STATUS "Configuring for LED Strip")
    target_sources(app PRIVATE src/status_light_strip.cpp)
elseif(has_led)
    message(STATUS "Configuring for GPIO LED")
    target_sources(app PRIVATE src/status_light_gpio.cpp)
else()
    message(FATAL_ERROR "No hardware alias found! (led-strip or led0)")
endif()
