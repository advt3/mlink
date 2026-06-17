# Firmware Developer Guide

Instructions to set up, configure, build, flash, and monitor the Zephyr-based firmware for the Raspberry Pi Pico W and ESP32-C6 DevKit.

---

## 1. Credentials Configuration (1Password)

To prevent WiFi and MQTT credentials from being committed to Git, they are configured in a Git-ignored `device/firmware/local.conf` overlay. 

You can read the deployed cloud broker credentials dynamically from 1Password and populate the file:

```bash
# 1. Initialize local.conf with WiFi settings (clears old configurations if run again)
cd device/firmware
cat <<EOF > local.conf
# Local configuration overlay (ignored by Git)
CONFIG_APP_WIFI_SSID="your-wifi-ssid"
CONFIG_APP_WIFI_PASSWORD="your-wifi-password"
EOF

# 2. Retrieve the cloud broker domain, username, and password from 1Password
export MQTT_DOMAIN=$(op read "op://Automation/environment-measures-mqtt-auth/domain")
export MQTT_USER=$(op read "op://Automation/environment-measures-mqtt-auth/username")
export MQTT_PASS=$(op read "op://Automation/environment-measures-mqtt-auth/password")

# 3. Append them to local.conf
echo "CONFIG_APP_MQTT_BROKER_HOST=\"$MQTT_DOMAIN\"" >> local.conf
echo "CONFIG_APP_MQTT_BROKER_PORT=8883" >> local.conf
echo "CONFIG_APP_MQTT_USERNAME=\"$MQTT_USER\"" >> local.conf
echo "CONFIG_APP_MQTT_PASSWORD=\"$MQTT_PASS\"" >> local.conf
```

---

## 2. Compile & Flash

Ensure your Zephyr virtual environment is active:
```bash
source ~/zephyrproject/.venv/bin/activate
source ~/zephyrproject/zephyr/zephyr-env.sh
```

### 1. Download/Extract the Root CA or Server Certificate
Before building, make sure the CA certificate is formatted into the source directory (`device/firmware/src/ca_cert.inc`). You can do this in two ways:

- **Method A: Let's Encrypt Root CA (Default)**
  Downloads the Let's Encrypt `ISRG Root X1` certificate and formats it:
  ```bash
  cd infrastructure/stack
  make get-root-ca
  ```

- **Method B: Extract directly from the live broker**
  If using custom or self-signed certs, extract and format the certificate directly from the broker (run from repository root):
  ```bash
  openssl s_client -showcerts -connect metrics.advt3.com:8883 </dev/null 2>/dev/null | openssl x509 -outform PEM | awk '{print "\"" $0 "\\n\""}' > device/firmware/src/ca_cert.inc
  ```

### 2. Build ESP32 Firmware
From the `device/firmware` directory, compile the binary for the ESP32-C6:
```bash
cd device/firmware
make build-esp32
```

### 3. Build Pico W Firmware
Compile the binary for the Raspberry Pi Pico W:
```bash
cd device/firmware
make build-pico
```

### 4. Flash the Firmware
Connect the target device (ESP32 or Pico W) and run:
```bash
make flash
```

---

## 3. Monitoring Console Output

To watch the live connection logs and verify that the firmware is successfully establishing a secure TLS connection to the cloud broker and sending MQTT messages:

```bash
make monitor
```
*(Uses `tio` to connect to the automatically detected serial debug console).*
