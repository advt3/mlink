#include "mqtt_publisher.hpp"
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_DECLARE(mlink, LOG_LEVEL_INF);

// Zephyr JSON descriptor for SensorReading
struct json_sensor_reading {
    double temperature;
    double humidity;
    double pressure;
    int address;
    const char *mac;
};

static const struct json_obj_descr json_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct json_sensor_reading, temperature, JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(struct json_sensor_reading, humidity, JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(struct json_sensor_reading, pressure, JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(struct json_sensor_reading, address, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct json_sensor_reading, mac, JSON_TOK_STRING),
};

MqttPublisher::MqttPublisher(const char *client_id) : connection_(client_id) {}

bool MqttPublisher::initialize() {
    return connection_.initialize();
}

bool MqttPublisher::start() {
    return connection_.start();
}

void MqttPublisher::stop() {
    connection_.stop();
}

bool MqttPublisher::get_connected() {
    return connection_.get_connected();
}

bool MqttPublisher::publish(const SensorReading& reading) {
    if (!connection_.get_connected()) {
        LOG_WRN("Cannot publish: Broker is offline.");
        return false;
    }

    struct json_sensor_reading j_read = {
        .temperature = reading.temperature,
        .humidity = reading.humidity,
        .pressure = reading.pressure,
        .address = reading.address,
        .mac = (const char *)connection_.get_client()->client_id.utf8
    };

    char json_buf[256];
    ssize_t json_len = json_obj_encode_buf(json_descr, ARRAY_SIZE(json_descr),
                                           &j_read, json_buf, sizeof(json_buf));
    if (json_len < 0) {
        LOG_ERR("JSON encoding failed: %d", (int)json_len);
        return false;
    }

    struct mqtt_publish_param param{};
    param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
    param.message.topic.topic.utf8 = (const uint8_t *)CONFIG_APP_MQTT_TOPIC;
    param.message.topic.topic.size = strlen(CONFIG_APP_MQTT_TOPIC);
    param.message.payload.data = (uint8_t *)json_buf;
    param.message.payload.len = strlen(json_buf);
    param.message_id = k_uptime_get_32();

    k_mutex_lock(connection_.get_mutex(), K_FOREVER);
    int rc = mqtt_publish(connection_.get_client(), &param);
    k_mutex_unlock(connection_.get_mutex());

    if (rc != 0) {
        LOG_ERR("mqtt_publish failed direct write: %d", rc);
        return false;
    }

    return true;
}
