package main

import (
	"log"
	"runtime"

	"desk-link/internal/config"
	"desk-link/internal/gui"
	"desk-link/internal/mqtt"
)

var (
	lastMessage = ""
	connStatus  = "Connecting..."
)

// refresh combines connection status and the last received message to update the display.
func refresh() {
	if connStatus == "Connected" {
		if lastMessage == "" {
			gui.UpdateText("Waiting for data...")
		} else {
			gui.UpdateText(lastMessage)
		}
	} else {
		gui.UpdateText(connStatus)
	}
}

func main() {
	// Cocoa requires the main event loop to run on the main OS thread.
	runtime.LockOSThread()

	// Load configuration
	cfg, err := config.LoadConfig("config.json")
	if err != nil {
		log.Fatalf("Error loading config.json: %v", err)
	}

	// Initialize MQTT connection with callbacks in a functional style
	client, err := mqtt.ConnectAndSubscribe(
		cfg,
		func(msg string) {
			lastMessage = msg
			refresh()
		},
		func(status string) {
			connStatus = status
			refresh()
		},
	)
	if err != nil {
		log.Fatalf("Error initializing MQTT: %v", err)
	}
	defer client.Disconnect(250)

	log.Printf("Starting macOS desktop widget window...")
	// Start the native Cocoa GUI. This blocks until the application quits.
	gui.Start(cfg.UI)
}
