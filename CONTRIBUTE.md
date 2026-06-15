# Contributing

This document outlines how to contribute to the Environment Metrics Link project.

## Development Workflow

### 1. Environment Setup
Ensure you have the **Zephyr RTOS** environment installed. 
- Follow the instructions in `device/firmware/README.md` to configure your virtual environment and install dependencies like `esptool`.
- For first-time setup guidance on Raspberry Pi Pico, refer to this [comprehensive guide](https://advt3.com/posts/zephyr-freestanding-cpp-rpi-pico-v4/).

### 2. Building and Testing
- Always use the provided `Makefile` for builds to ensure consistency across different hardware targets.
- Verify your changes on at least one hardware target (ESP32-C6 or Raspberry Pi Pico) before submitting.

## Coding Conventions

We follow these standards to ensure the codebase remains maintainable and hardware-agnostic.

### Naming Conventions
We prefer **functional naming** over technical naming. Names should describe *what* a component does rather than *how* it is implemented (e.g., `status_light.hpp` is functional, `status_light_gpio.cpp` is a technical implementation).

### File Structure & Naming
- **Interfaces**: Functional name (e.g., `status_light.hpp`).
- **Implementations**: `[functional_name]_[technical_detail].cpp` (e.g., `status_light_gpio.cpp`).
- **Files & Directories**: Always use `snake_case`.

### Code Style
- **Classes**: `PascalCase`.
- **Methods**: `snake_case` (matching Zephyr style).
- **Members**: Use `m_` prefix (e.g., `m_initialized`).

### Hardware Abstraction
Core logic must remain "unpolluted" by hardware-specific code. Use CMake build-time selection instead of preprocessor directives (`#if`, `#ifdef`) whenever possible.
