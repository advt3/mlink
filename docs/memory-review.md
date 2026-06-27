# ESP32-C6 Memory Review (mlink)

This document provides a breakdown of the SRAM allocations for the `mlink` firmware on the ESP32-C6 DevKitC to prevent runtime heap exhaustion and boot crashes.

---

## 1. ESP32-C6 Memory Limitations
The ESP32-C6 has a total of **320 KB of SRAM**. This SRAM is shared between:
1. **Static Data & BSS:** Global variables and static buffers (including thread stacks).
2. **Zephyr Kernel Core:** OS kernel objects, schedulers, and network interface descriptors.
3. **Espressif WiFi/Bluetooth Blobs:** The binary drivers require **~60 KB to 80 KB** of SRAM for RX/TX buffers, DMA, and internal controller state.
4. **System Heap (`CONFIG_HEAP_MEM_POOL_SIZE`):** Used for runtime allocations like network packets, sockets, and cJSON parsing.
5. **MbedTLS Heap (`CONFIG_MBEDTLS_HEAP_SIZE`):** Dedicated buffer heap used by MbedTLS for TLS handshakes and session contexts.

---

## 2. Allocation Breakdown

### Heap Memory
| Parameter | Size (Previous) | Size (Optimized) | Description |
| :--- | :--- | :--- | :--- |
| `CONFIG_HEAP_MEM_POOL_SIZE` | 160 KB | **64 KB** | General system heap (JSON, net packets, sockets) |
| `CONFIG_MBEDTLS_HEAP_SIZE` | 96 KB | **64 KB** | Heap dedicated for TLS handshakes (MQTT TLS + Mender HTTPS) |
| **Total Heaps** | **256 KB** | **128 KB** | **Saves 128 KB of SRAM** |

### Static Thread Stacks
Thread stacks are allocated statically in BSS at compile time:
*   `CONFIG_MAIN_STACK_SIZE`: **8 KB**
*   `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`: **4 KB**
*   `CONFIG_NET_MGMT_EVENT_STACK_SIZE`: **4 KB**
*   `CONFIG_NET_RX_STACK_SIZE`: **4 KB**
*   `CONFIG_NET_TX_STACK_SIZE`: **4 KB**
*   `conn_thread_stack` (WiFi Manager): **4 KB**
*   `mqtt_thread_stack` (MQTT/Telemetry): **8 KB**
*   `mender_client_stack` (Mender Client): **8 KB**
*   **Total Static Stacks:** **44 KB**

---

## 3. Why the Previous Config Crashed
With Heaps (256 KB) + Static Stacks (44 KB), the total memory reservation reached **300 KB**.
This left only **20 KB** for the Zephyr kernel, global variables, and the Espressif WiFi driver. As a result, when the WiFi driver initialized and attempted to allocate its DMA buffers, the allocation failed or corrupted the heap, causing the ESP32-C6 to freeze/crash on boot.

---

## 4. Tuning Recommendations
If you experience out-of-memory errors during concurrent TLS operations:
1.  **Do not increase both heaps simultaneously.** Keep the total sum of `CONFIG_HEAP_MEM_POOL_SIZE` + `CONFIG_MBEDTLS_HEAP_SIZE` below **140 KB**.
2.  **MbedTLS Optimization:** If memory is tight, you can reduce `CONFIG_MBEDTLS_HEAP_SIZE` to **48 KB** (49152), which is the absolute minimum safe size for standard 2048-bit RSA/ECDSA TLS handshakes in Zephyr.
