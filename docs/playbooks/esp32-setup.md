# Developing a Zephyr Freestanding C++ Application on ESP32 (v4.4.0)

This guide walks you through setting up a C++20-ready freestanding Zephyr project for the **Espressif ESP32 series** (focusing on the RISC-V **ESP32-C6**, with notes for Xtensa-based variants) using the latest **Zephyr v4.4.0**.

The freestanding pattern keeps your application in its own Git repository, separate from the Zephyr source tree, simplifying version control.

---

## Requirements
*   **Host OS**: macOS or Linux
*   **Target Board**: ESP32-C6 DevKitC (or other ESP32 family boards)
*   **USB-to-UART/JTAG connection** for flashing and console logs

---

## Step 1 – Workspace & Toolchain Setup

Run the following commands to create an isolated workspace, fetch Espressif hardware modules (narrowed to save space), and install the compiler:

```bash
# 1. Create virtual environment and workspace
mkdir ~/zephyrproject && cd ~/zephyrproject
python3 -m venv .venv
source .venv/bin/activate
pip install west

# 2. Initialize the workspace (pinned to Zephyr v4.4.0)
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0 .

# 3. Configure project filter to only download required modules (saves space/time)
west config manifest.project-filter "-.*, +hal_espressif, +mbedtls, +tf-psa-crypto"
west update

# 4. Register Zephyr with CMake and install Python requirements
west zephyr-export
west packages pip --install

# 5. Download the proprietary firmware binary blobs for the ESP32 series
west blobs fetch hal_espressif

# 6. Install the cross-compiler SDK
cd zephyr
# For RISC-V based ESP32 (ESP32-C6):
west sdk install --toolchains riscv64-zephyr-elf --version 1.0.1

# (Optional) For Xtensa-based ESP32 (ESP32, ESP32-S3):
# west sdk install --toolchains xtensa-espressif-elf --version 1.0.1
```

---

## Step 2 – Verify Tool Versions

Ensure your host tool versions match the baseline requirements:
*   **Python**: `>= 3.12`
*   **CMake**: `>= 3.20.0`
*   **West**: `>= 1.5.0`

---

## Step 3 – Create a New Project

Clone your application repository (e.g., `mlink`) outside the `~/zephyrproject` directory. Your repository should contain your `CMakeLists.txt`, `prj.conf`, and `src/` directory.

---

## Step 4 – Sourcing the Environment

Before building, activate your Python virtual environment and source Zephyr's internal environment script to load all required paths:

```bash
source ~/zephyrproject/.venv/bin/activate
source ~/zephyrproject/zephyr/zephyr-env.sh
```

---

## Step 5 – Build, Flash, and Log

Use the `west` meta-tool to compile, program, and monitor your ESP32-C6:

### 1. Build
Compile the application using the ESP32-C6 target board (and `--sysbuild` if MCUboot integration is enabled):

*   **Clean Build** (required if configurations change):
    ```bash
    west build -p always -b esp32c6_devkitc/esp32c6/hpcore -d build --sysbuild
    ```
*   **Incremental Build**:
    ```bash
    west build -d build
    ```

### 2. Flash
Program the binary to the ESP32-C6:
```bash
west flash -d build
```

### 3. Log
Monitor the serial logs via `tio` or another serial terminal:
```bash
# macOS serial port pattern (replace with your active serial port)
tio /dev/tty.usbserial-*
```

---

## C++ Integration Notes

*   **Enable C++**: Add the following options to your `prj.conf`:
    ```kconfig
    CONFIG_CPP=y
    CONFIG_STD_CPP20=y
    CONFIG_REQUIRES_FULL_LIBCPP=y
    ```
*   **Static Initialization**: Avoid static local objects inside your thread loops or initialization paths that require guarded initialization unless thread safety guarantees are handled.
*   **Threading**: Use Zephyr RTOS native APIs (`k_thread_create`, `k_work`) for multitasking rather than `std::thread` to respect microcontroller resource constraints.
