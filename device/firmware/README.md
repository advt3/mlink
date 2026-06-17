# firmware

This folder contains the edge device firmware implemented in C++20 using the **Zephyr RTOS** framework.

### 3. Monitor Serial Output
Connect to the device's serial console (requires `tio` installed):
```bash
make monitor
```
*Note: You can override the device path if needed: `make monitor SERIAL_DEV=/dev/tty.usbmodem123`*

For detailed hardware setup instructions (wiring, debug probe), refer to [docs/01-hardware-setup.md](../../docs/01-hardware-setup.md).

## Prerequisites

### Installing & Updating Zephyr Modules (Drivers/Blobs)
If you are missing the platform-specific HALs or binary blobs for WiFi/Bluetooth drivers, run the following commands from your Zephyr workspace directory (e.g., `~/zephyrproject`):

#### 1. ESP32 Targets (hal_espressif)
```bash
cd ~/zephyrproject
source .venv/bin/activate
west update hal_espressif
west blobs fetch hal_espressif
```

#### 2. Raspberry Pi Pico W Targets (CYW43439 & hal_infineon)
```bash
cd ~/zephyrproject
source .venv/bin/activate
west update cmsis_6 hal_rpi_pico hal_infineon mbedtls tf-psa-crypto
west blobs fetch hal_infineon
```

### ESP32-C6 Target Tooling
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

## Unit Testing & Code Coverage

Tests run locally on the host computer via Zephyr's `native_sim` architecture.

> [!NOTE]
> Zephyr's POSIX architecture (`native_sim`) is only supported on **Linux**. If you are developing on macOS or Windows, you will need to run these test commands inside a Linux virtual machine or Docker container.

### Run Unit Tests
To compile and execute the test suites using `ZTest`:
```bash
make test
```

### Generate Coverage Reports
To compile with GCC coverage flags (`gcov`), run tests, and generate an HTML report:
```bash
make coverage
```
The resulting coverage HTML page will be available under:
`tests/filter/build/coverage-html/index.html`

