# Technical Description: Metric Link (Field-to-Cloud Data Foundations)

Metric Link is a functional project that implements a secure, low-power data foundation for environmental monitoring.

## 1. Field Architecture (Firmware)

The firmware is built using **Zephyr RTOS** and **C++20**, targeting the Raspberry Pi Pico W and ESP32.

### Key Components:
- **RTOS Kernel:** Uses preemptive multithreading to separate time-critical sensor sampling from network-bound data movement.
- **Networking Stack:**
  - **WiFi:** Native Zephyr Wi-Fi management (`CONFIG_WIFI`).
  - **Transport:** MQTT client library (`CONFIG_MQTT_LIB`) secured via **MbedTLS** (`CONFIG_MQTT_LIB_TLS`).
  - **Trust Store:** The Let's Encrypt ISRG Root X1 certificate is embedded in the binary via `ca_cert.inc` to enable TLS server verification.
- **Sensor Integration:**
  - Uses the Zephyr Sensor API for BME280/BMP280 over I2C.
  - Implements a modular "Runner" pattern in C++ to decouple sensor logic from the MQTT publisher.
- **Memory Management:** Dedicated MbedTLS heap (`CONFIG_MBEDTLS_HEAP_SIZE=65536`) to handle TLS handshake requirements on constrained memory.

## 2. Infrastructure Architecture (Cloud)

The infrastructure follows a **Cloud-Native Field** pattern, using Terraform for provisioning and Docker Compose for service orchestration.

### Provisioning (Terraform):
- **GCP Provider:** Provisions a custom VPC, Firewall rules, and a GCE micro-instance.
- **Security:** Ingress is strictly limited to MQTT TLS (8883) and SSH (22).
- **Automation:** Uses GCE startup scripts to install Docker and initialize the project stack.

### Service Stack (Docker Compose):
- **MQTT Broker:** Eclipse Mosquitto (configured for TLS and password authentication).
- **Ingestion:** Telegraf acts as the glue, subscribing to MQTT topics and writing to VictoriaMetrics.
- **Observability:**
  - **VictoriaMetrics:** High-efficiency time-series storage.
  - **vmalert/Alertmanager:** Executes alerting rules and routes notifications (e.g., to Telegram).

## 3. Operations and Secret Management

Operational security is maintained by strictly separating configuration from code.

- **1Password Integration:** The `op` CLI is used to fetch credentials, tokens, and certificates at deploy time.
- **Local Overrides:** Developers use `local.conf` (Firmware) and `.env` (Infrastructure) files that are excluded from version control.
- **Deployment:** A `Makefile`-driven workflow automates building the firmware, provisioning the cloud, and deploying the stack over SSH.

## 4. Scalability Path

While the project is optimized for small-to-medium deployments, the architecture is designed for growth:
- **Broker:** Swap Mosquitto for **EMQX** for high-availability and clustering.
- **Storage:** VictoriaMetrics can be transitioned from single-node to a clustered deployment.
- **Device Node:** The Zephyr-based firmware is portable to larger SoCs (e.g., i.MX series) if processing requirements increase.
