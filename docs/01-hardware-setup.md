# Hardware Setup & Wiring Guide

This guide covers the physical setup, wiring, and debugging configuration for the MetricLink firmware.

---

## 1. Peripheral Wiring

The following tables describe the connection between the microcontrollers and the external components (BME280 sensor and Status LEDs).

### Raspberry Pi Pico W
| Component      | Pin Function | Pico Pin | GPIO              |
|:---------------|:-------------|:---------|:------------------|
| **BME280**     | I2C1 SDA     | Pin 31   | GP26              |
| **BME280**     | I2C1 SCL     | Pin 32   | GP27              |
| **BME280**     | VCC (3.3V)   | Pin 36   | 3V3_OUT           |
| **BME280**     | GND          | Pin 38   | GND               |
| **Status LED** | GPIO         | Pin 22   | GP17 (Active Low) |

### ESP32-C6 DevKitC
| Component      | Pin Function | ESP32 Pin | GPIO   |
|:---------------|:-------------|:----------|:-------|
| **BME280**     | I2C0 SDA     | D6        | GPIO 6 |
| **BME280**     | I2C0 SCL     | D7        | GPIO 7 |
| **BME280**     | VCC (3.3V)   | 3V3       | 3.3V   |
| **BME280**     | GND          | GND       | GND    |
| **WS2812 LED** | I2S Data     | D8        | GPIO 8 |

---

## 2. Debugging with RPi Debug Probe (macOS)

The Raspberry Pi Debug Probe provides both SWD debugging and a UART serial console.

### Software Installation
Install **OpenOCD** and **tio** using Homebrew:
```bash
brew install openocd tio
```

### Hardware Connections
The Debug Probe has two ports: **D** (Debug / SWD) and **T** (Target / UART).

#### 1. SWD (Debugging)
Connect the **D** port to the **SWD** header on the Raspberry Pi Pico using the 3-pin JST-SH connector.

#### 2. UART (Serial Console)
Connect the **T** port to the Pico's UART pins using the split jumper cables:

| Debug Probe Pin | Wire Color | Pico Pin | Pico Function |
|:----------------|:-----------|:---------|:--------------|
| **RX**          | Orange     | Pin 1    | TX (GP0)      |
| **TX**          | Yellow     | Pin 2    | RX (GP1)      |
| **GND**         | Black      | Any      | GND           |

---

## 3. Serial Console Monitoring

Connect to the serial console using `tio` at 115200 baud:
```bash
# Detected as a usbmodem device on macOS
tio -b 115200 /dev/cu.usbmodem*
```

---

## 4. Launching the Debugger

To start the OpenOCD GDB server for the RP2040:
```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"
```
