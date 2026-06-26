package mqtt

import (
	"crypto/tls"
	"fmt"
	"log"
	"time"

	"desk-link/internal/config"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

// ConnectAndSubscribe initializes the MQTT client, handles automatic reconnection,
// and uses callbacks to report connection status and message payloads in a functional manner.
func ConnectAndSubscribe(cfg config.Config, onMessage func(string), onStatus func(string)) (mqtt.Client, error) {
	opts := mqtt.NewClientOptions()
	opts.AddBroker(cfg.Broker)
	opts.SetClientID(cfg.ClientID)
	
	if cfg.Username != "" {
		opts.SetUsername(cfg.Username)
	}
	if cfg.Password != "" {
		opts.SetPassword(cfg.Password)
	}

	// Auto-reconnect parameters
	opts.SetAutoReconnect(true)
	opts.SetMaxReconnectInterval(10 * time.Second)

	// TLS Config (needed for tcps:// and ssl:// connections)
	tlsConfig := &tls.Config{
		InsecureSkipVerify: cfg.TLSSkipVerify,
	}
	opts.SetTLSConfig(tlsConfig)

	// Subscription handler
	var subHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
		onMessage(string(msg.Payload()))
	}

	opts.OnConnect = func(client mqtt.Client) {
		onStatus("Connected")
		token := client.Subscribe(cfg.Topic, 1, subHandler)
		if token.Wait() && token.Error() != nil {
			onStatus(fmt.Sprintf("Sub error: %v", token.Error()))
		} else {
			log.Printf("Subscribed to topic: %s", cfg.Topic)
		}
	}

	opts.OnConnectionLost = func(client mqtt.Client, err error) {
		onStatus("Disconnected")
		log.Printf("MQTT connection lost: %v", err)
	}

	client := mqtt.NewClient(opts)
	token := client.Connect()
	
	// Perform initial connection check in background to not block GUI initialization
	go func() {
		if token.Wait() && token.Error() != nil {
			onStatus("Conn failed")
			log.Printf("MQTT connection failed: %v", token.Error())
		}
	}()

	return client, nil
}
