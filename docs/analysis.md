# Mender Integration Analysis & Findings (ESP32-C6)

This document summarizes the technical findings, diagnostics, and fixes implemented to integrate the Mender OTA client and Wi-Fi manager into the `mlink` project.

---

## 1. Socket Connection Timeout (`st=-116` / `ETIMEDOUT`)
*   **Symptom:** The DNS resolved `eu.hosted.mender.io` successfully, but the TCP socket connection repeatedly timed out after 3 seconds:
    ```
    zsock_received_cb ... ctx=0x40856ee8, pkt=0, st=-116, user_data=0
    Unable to connect to the host 'eu.hosted.mender.io:443', result = -1, error: Connection timed out
    ```
*   **Cause:** The ESP32-C6 resolved the dual-stack server name and received both IPv4 and IPv6 addresses. Since `CONFIG_NET_IPV6` was not disabled, the socket layer tried to connect via IPv6. On local networks with active LAN IPv6 but unrouted WAN IPv6 paths, the TCP handshake to the IPv6 address silently dropped and timed out.
*   **Fix:** Force IPv4-only operations by explicitly disabling the IPv6 stack in `prj.conf`:
    ```kconfig
    CONFIG_NET_IPV6=n
    ```

---

## 2. Boot Crashing / Freezing (SRAM Exhaustion)
*   **Symptom:** The board would boot, establish a Wi-Fi connection, and then instantly freeze or crash.
*   **Cause:** The ESP32-C6 has a physical limit of **320 KB of SRAM**. The previous Kconfig allocated 160 KB for the system heap and 96 KB for the MbedTLS heap, which along with thread stacks (44 KB) consumed **300 KB**. This left less than 20 KB for the Zephyr kernel and the Espressif WiFi driver blobs (which require ~60-80 KB), triggering allocation panics.
*   **Fix:** Optimized heap limits in `prj.conf` to free up **128 KB of SRAM**:
    ```kconfig
    CONFIG_HEAP_MEM_POOL_SIZE=65536    # 64 KB (down from 160 KB)
    CONFIG_MBEDTLS_HEAP_SIZE=65536     # 64 KB (down from 96 KB)
    ```

---

## 3. RISC-V Exception (`mcause: 2, Illegal instruction`)
*   **Symptom:** A CPU exception occurred immediately after Mender client activation:
    ```
    mcause: 2, Illegal instruction | mepc: 00000000 | ra: 00000000
    Current thread: 0x4083cd88 (unknown)
    ```
*   **Cause:** Stack overflow in the Mender scheduler thread. During the MbedTLS secure handshake, heavy elliptic-curve (ECDSA) cryptographic operations exceeded the default **6 KB** stack size of the Mender scheduler work queue, overwriting the return address (`ra`) to 0.
*   **Fix:** Configured a dedicated **12 KB stack** for the Mender task queue and increased the system work queue stack to 8 KB:
    ```kconfig
    CONFIG_MENDER_SCHEDULER_SEPARATE_WORK_QUEUE=y
    CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE=12
    CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=8192
    ```

---

## 4. Default log prefix date (`1970-01-01`)
*   **Symptom:** Log lines displayed `[1970-01-01T00:xx:xxZ]` in the prefix, even after the POSIX time was synchronized.
*   **Cause:** Zephyr logger timestamps are generated from system uptime (clock cycles since boot), meaning they remain relative to 1970 regardless of `clock_settime` updates.
*   **Fix:** Hooked a custom 1Hz log timestamp provider using `log_set_timestamp_func()` in `main.cpp`. Once the SNTP time synchronizes, the logging offset is updated to match the real Unix Epoch, updating the log prefixes to the correct calendar date (e.g. `[2026-06-27T17:15:xxZ]`).
