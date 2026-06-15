ZEPHYR_WORKSPACE ?= /Users/shurtado/zephyrproject
WEST ?= $(ZEPHYR_WORKSPACE)/.venv/bin/west
# Ensure venv tools are in PATH for CMake
export PATH := $(ZEPHYR_WORKSPACE)/.venv/bin:$(PATH)

APP_DIR ?= $(shell pwd)

.PHONY: build
build: build-esp32

.PHONY: build-pico
build-pico:
	cd $(ZEPHYR_WORKSPACE) && $(WEST) build -p always -b rpi_pico/rp2040/w -d $(APP_DIR)/build $(APP_DIR) -- -DEXTRA_DTC_OVERLAY_FILE=$(APP_DIR)/boards/rpi_pico_rp2040_w.overlay

.PHONY: build-pico-basic
build-pico-basic:
	cd $(ZEPHYR_WORKSPACE) && $(WEST) build -p always -b rpi_pico -d $(APP_DIR)/build $(APP_DIR) -- -DEXTRA_DTC_OVERLAY_FILE=$(APP_DIR)/boards/rpi_pico.overlay

.PHONY: build-esp32
build-esp32:
	cd $(ZEPHYR_WORKSPACE) && $(WEST) build -p always -b esp32c6_devkitc/esp32c6/hpcore -d $(APP_DIR)/build $(APP_DIR) -- -DEXTRA_DTC_OVERLAY_FILE=$(APP_DIR)/boards/esp32c6_devkitc.overlay -DCONF_FILE="prj.conf;boards/esp32c6_devkitc.conf"

.PHONY: flash
flash:
	cd $(ZEPHYR_WORKSPACE) && $(WEST) flash -d $(APP_DIR)/build

.PHONY: clean
clean:
	rm -rf build

SERIAL_DEV ?= $(shell for f in /dev/tty.usbmodem5ABA* /dev/tty.usbmodem* /dev/tty.usbserial-*; do if [ -e "$$f" ]; then echo "$$f"; break; fi; done)
BAUD ?= 115200

.PHONY: monitor
monitor:
	@if [ -z "$(SERIAL_DEV)" ]; then \
		echo "No serial device found! Checked /dev/tty.usbmodem* and /dev/tty.usbserial-*"; \
		exit 1; \
	fi
	tio -b $(BAUD) $(SERIAL_DEV)
