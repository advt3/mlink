# Mender OTA Integration Playbook (ESP32-C6)

This playbook outlines the exact step-by-step procedure to set up the Zephyr SDK, retrieve
the **Mender MCU Client**, configure **MCUboot** partitioning for the ESP32-C6, provision
the Hosted Mender tenant token, and build/flash the system.

> **Platform**: ESP32-C6 DevkitC only (RP2040/Pico W is not supported due to memory constraints).

---

## Step 0: Install Zephyr SDK 1.0.x

Zephyr 4.4 requires SDK ≥ 1.0. The SDK provides the cross-compilation toolchains. If already
installed (check `~/zephyr-sdk-1.0.x` exists), skip to Step 1.

```bash
# Download the macOS arm64 minimal bundle
SDK_VERSION=1.0.1
cd ~
curl -L -O "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK_VERSION}/zephyr-sdk-${SDK_VERSION}_macos-aarch64_gnu.tar.xz"

# Extract
tar xf zephyr-sdk-${SDK_VERSION}_macos-aarch64_gnu.tar.xz

# Run setup (registers SDK in CMake package registry so sysbuild sub-builds can find it)
cd ~/zephyr-sdk-${SDK_VERSION}
./setup.sh

# macOS Gatekeeper: remove quarantine if binaries are blocked
xattr -r -d com.apple.quarantine ~/zephyr-sdk-${SDK_VERSION}
```

> After `./setup.sh`, the SDK registers itself in `~/.cmake/packages/Zephyr-sdk/` so CMake
> (and the sysbuild MCUboot sub-build) can find it without any environment variable.

---

## Step 1: Add the Mender MCU Client to `west.yml`

The `mender-mcu` library version `v1.0.0` is the stable release for recent Zephyr versions.

1. Locate the manifest at `~/zephyrproject/zephyr/west.yml`.
2. Add the `mender-mcu` project entry to the `projects` list:

```yaml
    - name: mender-mcu
      path: modules/lib/mender-mcu
      revision: v1.0.0
      url: https://github.com/mendersoftware/mender-mcu.git
      import: true
```

3. Update the workspace to download all required modules:

```bash
cd ~/zephyrproject
west config manifest.project-filter "-.*, +hal_espressif, +mbedtls, +tf-psa-crypto, +mcuboot, +mender-mcu, +cjson-zephyr"
west update
west blobs fetch hal_espressif
```

---

## Step 2: Retrieve the Mender Tenant Token

Use the `op` CLI to fetch your Hosted Mender tenant token from 1Password and populate the
local, Git-ignored configuration file:

```bash
# Retrieve from 1Password (adjust item name as needed)
MENDER_TOKEN=$(op read "op://Private/Mender/tenant_token")

# Update the token in local.conf
sed -i '' "s|CONFIG_MENDER_SERVER_TENANT_TOKEN=.*|CONFIG_MENDER_SERVER_TENANT_TOKEN=\"${MENDER_TOKEN}\"|" \
  /Users/shurtado/Projects/me/mlink/device/firmware/local.conf
```

The `local.conf` file is already Git-ignored and pre-configured with:
```
CONFIG_MENDER_MCU_CLIENT=y
# CONFIG_MENDER_SERVER_HOST="hosted.mender.io" (Defined via CONFIG_MENDER_SERVER_HOST_US in prj.conf)
CONFIG_MENDER_SERVER_TENANT_TOKEN="${MENDER_TOKEN}"
```

---

## Step 3: Flash Partitions (Inherited Automatically)

The standard partition layout required by MCUboot on the ESP32-C6 is automatically provided
by the board's default DTS file (derived from `partitions_0x0_default_4M.dtsi` upstream):

| Partition           | Size    | Offset     |
|---------------------|---------|------------|
| `boot_partition`    | 64 KB   | `0x0`      |
| `slot0_partition`   | 1792 KB | `0x20000`  |
| `slot1_partition`   | 1792 KB | `0x1e0000` |
| `storage_partition` | 192 KB  | `0x3b0000` |

Since the offsets are pre-configured upstream, **no manual partition overlays** are needed in
`esp32c6_devkitc.overlay`.

---

## Step 4: Configure Software Cryptographic Identity

Generate an ECDSA P-256 private key for device identity. The Mender client uses this key
stored in NVS for TLS handshakes and device authentication with the Mender server.

```bash
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:prime256v1 \
  -out /Users/shurtado/Projects/me/mlink/device/firmware/mender_key.pem
```

The `mender_key.pem` is already Git-ignored and the key file is already present from the
initial setup.

---

## Step 5: Build and Flash using Sysbuild

Zephyr's **Sysbuild** orchestrates the compilation of MCUboot and your main application
together. Both binaries are built and flashed in a single command.

```bash
cd /Users/shurtado/Projects/me/mlink/device/firmware

# Build (compiles MCUboot + firmware together via Sysbuild)
make build-esp32

# Flash both MCUboot and firmware to the ESP32-C6
make flash

# Monitor serial output to confirm boot and Mender auth
make monitor
```

### Troubleshooting: SDK not found during Sysbuild

If CMake errors with `Could not find a package configuration file provided by "Zephyr-sdk"`,
the SDK was not registered via `./setup.sh`. Fix options:

**Option A** – Re-run the SDK setup script (preferred):
```bash
~/zephyr-sdk-1.0.1/setup.sh
```

**Option B** – Export `ZEPHYR_SDK_INSTALL_DIR` before building:
```bash
export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-1.0.1
make build-esp32
```

---

## Step 6: Verify Mender Device Registration

After flashing and booting:

1. Open [Hosted Mender](https://hosted.mender.io) → **Devices** tab.
2. A new device with type `esp32c6_devkitc` should appear with **Pending** status.
3. **Accept** the device to authorize it.
4. The device MAC address is used as the identity key (configured in `src/mender_client.cpp`).

---

## Mender Update Workflow

Once the device is accepted, deploy a firmware update:

1. Build a new version of the firmware (`make build-esp32`).
2. Package it as a Mender artifact using the Makefile shortcut (requires `mender-artifact` installed):
   ```bash
   make mender
   ```
   Or use the raw command:
   ```bash
   # Install mender-artifact tool if needed
   brew install mender-artifact  # or download from mender.io

   mender-artifact write module-image \
     --type zephyr-image \
     --device-type esp32c6_devkitc \
     --artifact-name firmware-v$(date +%Y%m%d) \
     --output build/firmware.mender \
     --file build/firmware/zephyr/zephyr.signed.bin
   ```
3. Upload the `.mender` artifact to Hosted Mender and deploy to the device.
4. The device downloads the artifact, writes it to `slot1_partition`, and reboots.
5. MCUboot validates the new image and marks it as confirmed.

---

## Step 1: Add the Mender MCU Client to `west.yml`

The `mender-mcu` library version `v1.0.0` is the stable release for recent Zephyr RTOS versions.

1. Locate the manifest file in your freestanding workspace (at `~/zephyrproject/zephyr/west.yml`).
2. Add the `mender-mcu` project entry to the `projects` list in `west.yml`:

```yaml
    - name: mender-mcu
      path: modules/lib/mender-mcu
      revision: v1.0.0
      url: https://github.com/mendersoftware/mender-mcu.git
      import: true
```

3. Update your workspace to download the Mender client module along with MCUboot and Espressif HAL dependencies:
```bash
cd ~/zephyrproject
west config manifest.project-filter "-.*, +hal_espressif, +mbedtls, +tf-psa-crypto, +mcuboot, +mender-mcu, +cjson-zephyr"
west update
west blobs fetch hal_espressif
```

---

## Step 2: Retrieve the Mender Tenant Token

Use the `op` CLI to fetch your Hosted Mender tenant token from 1Password and populate it in your local, Git-ignored configuration:

```bash
# Retrieve from 1Password (adjust item name as needed)
MENDER_TOKEN=$(op read "op://Private/Mender/tenant_token")

# Append/Update device/firmware/local.conf
cat <<EOF >> /Users/shurtado/Projects/me/mlink/device/firmware/local.conf

# Mender Configuration
CONFIG_MENDER_MCU_CLIENT=y
# CONFIG_MENDER_SERVER_HOST="hosted.mender.io" (Defined via CONFIG_MENDER_SERVER_HOST_US in prj.conf)
CONFIG_MENDER_SERVER_TENANT_TOKEN="${MENDER_TOKEN}"
EOF
```

---

## Step 3: Flash Partitions (Inherited Automatically)

The standard partition layout required by MCUboot on the ESP32-C6 is automatically provided by the board's default dts file (derived from `partitions_0x0_default_4M.dtsi` upstream). It defines:
*   `boot_partition`: 64 KB (at offset `0x0`)
*   `slot0_partition` (Active image): 1792 KB (at offset `0x20000`)
*   `slot1_partition` (Upgrade image): 1792 KB (at offset `0x1e0000`)
*   `storage_partition`: 192 KB (at offset `0x3b0000`)

Since the offsets are pre-configured in the upstream board DTS, **no manual partition overlays** are required in `esp32c6_devkitc.overlay`. Doing so will cause label duplicate errors during devicetree parsing.

---

## Step 4: Configure Software Cryptographic Identity

Generate an ECDSA P-256 private key locally. The Mender client will use this key stored in NVS to negotiate device identity and secure TLS handshakes with the Mender server.

1. Generate the key file:
```bash
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:prime256v1 -out /Users/shurtado/Projects/me/mlink/device/firmware/mender_key.pem
```

2. The client application code will read this key from the flash NVS during `MenderClient::initialize()`.

---

## Step 5: Build and Flash using Sysbuild

Zephyr's **Sysbuild** orchestrates the compilation of MCUboot and your main application binary together.

1. You can now use the updated `Makefile` to trigger the build (which automatically activates Sysbuild and compiles MCUboot alongside your application):
```bash
cd /Users/shurtado/Projects/me/mlink/device/firmware
make build-esp32
```

2. Flash both MCUboot and the application to the ESP32-C6:
```bash
make flash
```

3. Confirm that the application boots and attempts to authenticate with Hosted Mender:
```bash
make monitor
```
