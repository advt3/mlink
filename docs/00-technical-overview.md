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

## 5. Production Architectural Realities and Grounding Insights

When transitioning the Metric Link reference platform to high-consequence or highly regulated industrial environments, architects must account for explicit hardware constraints, firmware-level nuances, and security vectors uncovered in platform research:

### Silicon & Network Stack Constraints
- **CYW43439 Wi-Fi Firmware Limitations:** The Infineon wireless chip on the Pico W inherently filters incoming UDP and TCP packets that do not correspond to an already open, outbound socket initialized by the host microcontroller. While Metric Link's outward-bound telemetry model natively circumvents this, bidirectional command-and-control architectures or edge NAT routing will require specialized application-layer proxies.
- **RP2040 Flash Execution Context (XiP):** The RP2040 utilizes an Execute-in-Place framework over an external SPI bus. Implementing OTA pipelines requires manual linker script modifications to force critical flash-writing drivers, interrupt vectors, and context-switching routines entirely into internal SRAM to prevent immediate hard faults when the flash controller temporarily disables XiP during firmware swaps.
- **RP2350 Driver Immaturity:** Organizations evaluating forward migration to the RP2350 (Pico 2) will face fragmentation in upstream RTOS flash controller drivers, requiring explicit device tree adjustments and overlay re-engineering due to changing hardware peripheral interfaces.

### Data Modeling and Storage Efficiency
- **High-Cardinality Performance:** General-purpose relational extensions (e.g., TimescaleDB) create severe disk I/O bottlenecks and underutilize multi-core systems when ingesting millions of unique metric streams with deep label dimensions. VictoriaMetrics maintains efficient write throughput by maximizing parallel compute utilization and optimizing disk bandwidth.
- **Storage Footprint:** Native time-series compression algorithms in VictoriaMetrics significantly reduce bytes-per-datapoint requirements compared to traditional relational models, lowering long-term operating costs for multi-year historical logs.

### Advanced Security and Hardware Vulnerabilities
- **Microcontroller OTP & Glitching Risks:** Recent hardware security challenges demonstrated that internal microcontroller features (such as secure boot validation or internal OTP memory locks) are highly vulnerable to physical side-channel and fault injection attacks, such as voltage glitching. Attackers with physical access can force undefined states that reset debug ports and expose internal secrets.
- **Hardened Cryptographic Co-processors:** True compliance with **IEC 62443-4-2** component-level security mandates requires an external Secure Element (e.g., Microchip ATECC608 TrustFLEX or Maxim DS28S60) acting as a dedicated co-processor. Private keys remain contained within hardened silicon, executing mathematical signature verifications via I2C/SPI challenge-response loops without exposing keys to the main processor's volatile memory.
- **Regulatory Alignment:** Adherence to the European Union's **NIS2 Directive (Article 21)** requires mapping technical controls directly to the IEC 62443 series, enforcing strict data-at-rest/in-transit encryption (mbedTLS/TLS 1.3), continuous audit logging (minimum 12-month retention), and physical tamper resistance.

### Semantic Interoperability & Data Serialization
- **Protocol Buffers (Protobuf):** Replacing raw JSON payloads with Protobuf binary encoding delivers a significant reduction in wire-size and CPU overhead on constrained devices. Schema evolution is managed via `.proto` files compiled into both firmware (nanopb) and backend services, ensuring forward and backward compatibility as metric definitions expand.
- **Alternative Serialization Formats:** For deployments with stricter human-readability or tooling requirements, **MessagePack** offers a compact binary envelope compatible with existing JSON toolchains, while **CBOR** (RFC 7049) is well-suited to CoAP-based transports and aligns with IETF IoT standards. Format selection should be driven by device RAM constraints, broker capabilities, and downstream consumer ecosystems.
- **Decoupled Translation Loops:** Utilizing an intermediate rule engine (such as the EMQX integrated ingestion logic) allows binary-encoded streams to be intercepted, decoded, and reformatted automatically before pushing to the `vmagent` ingestion endpoint, preventing upstream schema modifications from impacting the physical field device.
