# firmware

This folder contains the edge device firmware implemented in C++20 using the **Zephyr RTOS** framework.

### 3. Monitor Serial Output
Connect to the device's serial console (requires `tio` installed):
```bash
make monitor
```
*Note: You can override the device path if needed: `make monitor SERIAL_DEV=/dev/tty.usbmodem123`*

## Prerequisites

### ESP32-C6 Target
Building for the ESP32-C6 requires `esptool`. 

1. **Install Dependencies**: If missing, run:
   ```bash
   cd /Users/shurtado/zephyrproject
   .venv/bin/west packages pip --install
   ```

2. **Environment Path**: The build system must find `esptool` in your `PATH`.
   - **Using Makefile**: The provided `Makefile` automatically adds the venv to your path.
   - **Using West directly**: You must source your virtual environment first:
     ```bash
     source /Users/shurtado/zephyrproject/.venv/bin/activate
     ```

## How to Build

Run the following build commands from the root directory of the workspace.

### 1. Build for Raspberry Pi Pico W
```bash
make build-pico
```

### 2. Build for ESP32-C6 DevKitC
```bash
make build-esp32
```

## How to Deploy

### Raspberry Pi Pico W
1. Connect the Pico to your host machine via USB while holding down the **BOOTSEL** button.
2. Mount the Pico as a mass storage device.
3. Copy the compiled `build/zephyr/zephyr.uf2` file to the Pico drive.

### ESP32-C6 DevKitC
Connect the board to your host machine via USB and flash it:
```bash
make flash
```
