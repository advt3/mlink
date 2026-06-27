# Over-the-Air (OTA) Firmware Updates using Mender

This document defines the architecture, flash memory partitioning, security model, and software integration path for implementing Over-the-Air (OTA) updates for the Metric Link edge nodes using the **Mender MCU Client** and **MCUboot**.

---

## 1. System Architecture & Flow

The firmware update loop involves the **Mender Management Server**, the **Metric Link Edge Node (Firmware Application)**, and the **MCUboot Bootloader** managing dual-slot A/B execution contexts.

### Component Interaction

```mermaid
sequence diagram
    autonumber
    participant Mender as Mender Management Server
    participant App as Firmware Application
    participant Flash as Flash Memory
    participant Boot as MCUboot Bootloader

    Note over App: Periodically polls Mender Server
    App->>Mender: Check for new Deployment (Inventory & Current Version)
    alt New Deployment Available
        Mender-->>App: Return Artifact URL & Signature
        App->>Flash: Open Slot 1 (Secondary Partition) for writing
        loop In chunks over HTTPS
            App->>Mender: Fetch image chunk
            Mender-->>App: Image chunk
            App->>Flash: Write chunk to Slot 1
        end
        App->>App: Verify downloaded image checksum/signature
        App->>Flash: Set Slot 1 status to "Pending/Upgrade" (boot_request_upgrade)
        App->>Boot: Reset device
        Boot->>Flash: Check boot image status
        Note over Boot: Detects pending upgrade in Slot 1
        Boot->>Flash: Swap Slot 0 (Active) & Slot 1 (Secondary)
        Boot->>App: Jump to Slot 0 (New Image)
        alt Boot and Connect Successful
            App->>Flash: Confirm new image (boot_write_img_confirmed)
            App->>Mender: Report deployment success
        else Boot fails or Network unreachable
            Note over Boot: Watchdog resets or no confirmation
            Boot->>Flash: Swap Slot 0 & Slot 1 back (Rollback)
            Boot->>App: Jump to Slot 0 (Old Known-Good Image)
        end
    else No Updates
        Mender-->>App: No deployment
    end
```

---

## 2. Flash Memory Partition Layouts

Dual-slot (A/B) partitioning is required to support fail-safe updates with rollback capabilities.

### 2.1 Raspberry Pi Pico W (RP2040 - 2MB External Flash) [WONT BE SUPPORTED AT THIS TIME]
The memory on the pico is very low, so for now we will not supported.

### 2.2 ESP32-C6 (4MB/8MB Flash DevKit)
The ESP32-C6 handles code execution and flash operations natively. The partition scheme is mapped in the devicetree overlay:

| Partition Name | Size | Purpose |
| :--- | :--- | :--- |
| `mcuboot` | 64 KB | MCUboot bootloader |
| `slot0_partition` | 1536 KB | Active firmware slot |
| `slot1_partition` | 1536 KB | Secondary firmware slot |
| `storage_partition` | 256 KB | NVS Storage |

---

## 3. Mender MCU Client Configuration

The integration will use the standard **mender-mcu** module in Zephyr.

### 3.1 Kconfig Configuration (`prj.conf`)
Add the following configuration variables to enable MCUboot and Mender operations:

```ini
# Enable MCUboot integration
CONFIG_BOOTLOADER_MCUBOOT=y
CONFIG_MCUBOOT_IMG_MANAGER=y
CONFIG_IMG_MANAGER=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y

# Enable Mender Client
CONFIG_MENDER_MCU=y
CONFIG_MENDER_SERVER_HOST="hosted.mender.io"
CONFIG_MENDER_TENANT_TOKEN="YOUR_TENANT_TOKEN_HERE"
CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL=3600
CONFIG_MENDER_CLIENT_INVENTORY_POLL_INTERVAL=28800
```

---

## 4. Security & Compliance (NIS2 & IEC 62443-4-2)

### 4.1 Cryptographic Identity & Key Storage
To align with the **NIS2 Directive (Article 21)** and **IEC 62443-4-2** component-level security mandates, devices must authenticate uniquely.
*   **Production Deployment (Hardened):** Use an external Secure Element (e.g., **ATECC608**) to store the device's private key. Mathematical handshake signatures are generated inside the crypto-coprocessor via I2C without exposing keys to SRAM.
*   **Software Fallback (Development):** Store the RSA/ECDSA private key in the Zephyr NVS (`storage_partition`) with flash decryption enabled (for targets supporting hardware flash decryption like ESP32-C6).

### 4.2 Image Verification
MCUboot must verify the digital signature of the incoming update binary before booting:
*   Build artifacts are signed using `imgtool` with a private key (RSA 2048 or ECDSA P-256).
*   The corresponding public key is compiled into the MCUboot bootloader binary.
*   Images that fail signature verification are rejected and not booted.

---

## 5. Software Integration Plan

We will add a new component, `MenderClient`, implemented as an asynchronous runner to handle authentication, heartbeat, inventory reporting, and firmware download triggers.

### C++ Interface (`mender_client.hpp`)

```cpp
#pragma once

#include <zephyr/kernel.h>
#include <mender-mcu/client.h>

class MenderClient {
public:
    MenderClient();
    ~MenderClient() = default;

    bool initialize();
    void start();

private:
    static int identity_callback(char *identity, size_t max_len);
    static int inventory_callback(char *inventory, size_t max_len);
    static void deployment_status_callback(mender_deployment_status_t status);

    k_tid_t m_thread_id;
    struct k_thread m_thread_data;
    K_KERNEL_STACK_MEMBER(m_stack, 4096);

    static void thread_entry(void *p1, void *p2, void *p3);
};
```

---

## 6. Walkthrough & Testing Strategy

1.  **Virtual Simulation:** Compile the app for the `native_sim` target with simulated flash. Run upgrade cycles locally using a mock HTTP server.
2.  **Dev Board Testing:** Flash MCUboot and the initial firmware image to the ESP32-C6 or RP2040 W, generate a `.mender` artifact, deploy it to Mender cloud, and trigger a rollout.
3.  **Rollback Validation:** Intentionally flash a corrupt/non-connecting image to Slot 1, and verify that MCUboot rolls back safely to Slot 0 after watchdog expiry.
