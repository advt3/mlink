# Firmware

This folder contains the edge device firmware implemented in C++20 using the **Zephyr RTOS** framework.

## Quick Start

### 1. Build
Build the firmware for the respective devices, use `make build` for incremental builds.

```bash
make build-pico   # For Raspberry Pi Pico W
make build-esp32  # For ESP32-C6 DevKitC
```

### 2. Flash
Flash the compiled firmware to the respective devices using the following command:

```bash
make flash
```

### 3. Monitor


```bash
make monitor
```

## Documentation

For detailed instructions on setup, configuration, building, and testing, refer to the [Firmware Developer Guide](../../docs/02-firmware.md).

Refer to [docs/01-hardware-setup.md](../../docs/01-hardware-setup.md) for wiring and debug probe setup.
