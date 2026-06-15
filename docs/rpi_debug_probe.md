# Raspberry Pi Debug Probe Setup on macOS

This guide explains how to set up the Raspberry Pi Debug Probe on macOS for hardware debugging (via SWD) and console output (via UART) with a Raspberry Pi Pico.

---

## 1. Software Installation

Install **OpenOCD** (supporting CMSIS-DAP and RP2040) and **tio** (serial terminal) using Homebrew:

```bash
brew install openocd tio
```

> [!NOTE]
> Upstream OpenOCD (version 0.12.0 and later) has built-in support for both CMSIS-DAP and the RP2040 target, so building from source is generally no longer required.

---

## 2. Hardware Wiring

The Debug Probe has two ports: **D** (Debug / SWD) and **T** (Target / UART).

### SWD (Debugging) Connection
* Connect the **D** port on the Debug Probe to the **SWD** header on the Raspberry Pi Pico using the 3-pin JST-SH connector.

### UART (Serial Console) Connection
Connect the **T** port on the Debug Probe to the Pico's UART pins using the split jumper cables:
* **RX** (Orange wire) &rarr; Pico **TX** (GP0 / Pin 1)
* **TX** (Yellow wire) &rarr; Pico **RX** (GP1 / Pin 2)
* **GND** (Black wire) &rarr; Pico **GND** (Any ground pin, e.g., Pin 3)

Finally, plug the Debug Probe into your Mac using a micro-USB cable. Make sure the target Pico is also powered.

---

## 3. Serial Console Monitoring

The Debug Probe exposes a virtual serial port on macOS. You can find it by running:

```bash
ls /dev/cu.usbmodem*
```

To connect to the serial console, use `tio` (configured for a 115200 baud rate):

```bash
tio -b 115200 /dev/cu.usbmodem*
```

---

## 4. Debugging with OpenOCD

To launch the OpenOCD GDB server, execute the following command:

```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"
```

* **`interface/cmsis-dap.cfg`**: Configures OpenOCD to use the Debug Probe (which uses the CMSIS-DAP protocol).
* **`target/rp2040.cfg`**: Tells OpenOCD that the target chip is an RP2040.

---
