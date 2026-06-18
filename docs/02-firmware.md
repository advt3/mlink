# Firmware Developer Guide

Instructions to set up, configure, build, flash, and monitor the Zephyr-based firmware for the Raspberry Pi Pico W and ESP32-C6 DevKit.

For a comprehensive guide on setting up Zephyr with C++ on the Raspberry Pi Pico, refer to the [starting point guide](https://advt3.com/posts/zephyr-freestanding-cpp-rpi-pico-v4/).

---

## 1. Prerequisites

### Zephyr Environment
Ensure you have the **Zephyr RTOS** environment installed. 

### Installing & Updating Zephyr Modules (Drivers/Blobs)
If you are missing the platform-specific HALs or binary blobs for WiFi/Bluetooth drivers, run the following commands from your Zephyr workspace directory:

#### ESP32 Targets (hal_espressif)
```bash
west update hal_espressif
west blobs fetch hal_espressif
```

#### Raspberry Pi Pico W Targets (CYW43439 & hal_infineon)
```bash
west update cmsis_6 hal_rpi_pico hal_infineon mbedtls tf-psa-crypto
west blobs fetch hal_infineon
```

### ESP32-C6 Target Tooling
Building for the ESP32-C6 requires `esptool`.

1. **Install Dependencies**:
   ```bash
   west packages pip --install
   ```

2. **Environment Path**: The provided `Makefile` automatically adds the venv to your path. If using `west` directly, ensure your virtual environment is active.

---

## 2. Credentials Configuration

To prevent WiFi and MQTT credentials from being committed to Git, they are configured in a Git-ignored `device/firmware/local.conf` overlay. 

```bash
cd device/firmware
cat <<EOF > local.conf
# Local configuration overlay (ignored by Git)
CONFIG_APP_WIFI_SSID="your-wifi-ssid"
CONFIG_APP_WIFI_PASSWORD="your-wifi-password"
EOF
```

Refer to the [Infrastructure & Operations Guide](./03-infrastructure.md) for instructions on retrieving the cloud broker credentials from 1Password.

---

## 3. Build & Flash

### 1. Download/Extract the Root CA
Before building, make sure the CA certificate is formatted into `device/firmware/src/ca_cert.inc`:

```bash
# Method A: Let's Encrypt Root CA
cd infrastructure/stack
make get-root-ca

# Method B: Extract from live broker
openssl s_client -showcerts -connect <YOUR_BROKER_DOMAIN>:8883 </dev/null 2>/dev/null | openssl x509 -outform PEM | awk '{print "\"" $0 "\\n\""}' > device/firmware/src/ca_cert.inc
```

### 2. Build Firmware
```bash
cd device/firmware
make build-pico   # For Raspberry Pi Pico W
make build-esp32  # For ESP32-C6 DevKitC
```

### 3. Flash & Deploy

#### ESP32-C6 DevKitC
Connect via USB and flash:
```bash
make flash
```

#### Raspberry Pi Pico W
1. Connect via USB while holding **BOOTSEL**.
2. Mount the Pico as a mass storage device.
3. Copy `build/zephyr/zephyr.uf2` to the Pico drive.

---

## 4. Monitoring Console Output

To watch the live connection logs:
```bash
make monitor
```

---

## 5. Unit Testing & Code Coverage

Tests run locally on the host computer via Zephyr's `native_sim` architecture.

> [!IMPORTANT]
> `native_sim` is only supported on **Linux**. For macOS/Windows, use a Linux VM or Docker container.

### Run Unit Tests
```bash
make test
```

### Generate Coverage Reports
```bash
make coverage
```
Report location: `tests/filter/build/coverage-html/index.html`
