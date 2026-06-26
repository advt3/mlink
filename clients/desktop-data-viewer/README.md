# Desk-Link: macOS MQTT Desktop Widget

A native macOS application written in Go that connects to an MQTT broker using TLS with username and password, and projects the received text payload directly onto your desktop as a transparent, frameless, and draggable floating text widget.

Built using standard macOS Cocoa APIs (via CGO/Objective-C) to ensure it is lightweight and performs efficiently with no extra UI library dependencies.

## Prerequisites

- **macOS** (10.15+)
- **Go** (1.20+)
- **Xcode Command Line Tools** (for CGO/Clang compilation)

## Setup & Configuration

Configure the widget parameters in `config.json` located in the root directory:

```json
{
  "broker": "tcps://your-mqtt-broker:8883",
  "client_id": "desk-link-widget",
  "username": "your_username",
  "password": "your_password",
  "topic": "sensors/environment",
  "tls_skip_verify": true,
  "ui": {
    "font_size": 42,
    "text_color": "#FFD700",
    "width": 600,
    "height": 180,
    "x": 200,
    "y": 200
  }
}
```

### UI Options
- `font_size`: Size of the widget text on screen.
- `text_color`: Hex code specifying the text color (supports both `#FFF` and `#FFFFFF` formats).
- `width` / `height`: Dimensions of the text bounding box.
- `x` / `y`: Initial screen coordinates for the window (measured from the bottom-left corner of the screen).

## Usage

Use the provided `Makefile` to manage the lifecycle of the application:

### 1. Download Dependencies
```bash
make deps
```

### 2. Build the Application
```bash
make build
```

### 3. Run the Widget
```bash
make run
```

*Note: You can click and drag anywhere on the widget text to move it around your desktop.*

## Architecture

- **Unidirectional Event Flow**: Incoming MQTT message triggers a Go callback, which updates the internal state and dispatches the rendering action to the native Cocoa main thread via `dispatch_async`.
- **Zero GUI Dependencies**: Bridges directly to Apple's native AppKit frame using a compact, self-contained Objective-C implementation (`cocoa_gui.m`).
