package config

import (
	"encoding/json"
	"os"
)

// UIConfig represents configuration for the desktop window UI.
type UIConfig struct {
	FontSize  int    `json:"font_size"`
	TextColor string `json:"text_color"` // Hex representation e.g. #FFD700
	Width     int    `json:"width"`
	Height    int    `json:"height"`
	X         int    `json:"x"`
	Y         int    `json:"y"`
}

// Config holds all the application settings.
type Config struct {
	Broker        string   `json:"broker"`
	ClientID      string   `json:"client_id"`
	Username      string   `json:"username"`
	Password      string   `json:"password"`
	Topic         string   `json:"topic"`
	TLSSkipVerify bool     `json:"tls_skip_verify"`
	UI            UIConfig `json:"ui"`
}

// LoadConfig reads and parses a JSON config file.
func LoadConfig(path string) (Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return Config{}, err
	}

	var cfg Config
	if err := json.Unmarshal(data, &cfg); err != nil {
		return Config{}, err
	}

	// Set sensible defaults if not defined
	if cfg.UI.FontSize <= 0 {
		cfg.UI.FontSize = 36
	}
	if cfg.UI.TextColor == "" {
		cfg.UI.TextColor = "#FFFFFF"
	}
	if cfg.UI.Width <= 0 {
		cfg.UI.Width = 500
	}
	if cfg.UI.Height <= 0 {
		cfg.UI.Height = 150
	}

	return cfg, nil
}
