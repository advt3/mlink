.PHONY: build run clean deps

BINARY_NAME=desk-link

build:
	go build -o $(BINARY_NAME) cmd/desk-link/main.go

run: build
	./$(BINARY_NAME)

clean:
	rm -f $(BINARY_NAME)

deps:
	go get github.com/eclipse/paho.mqtt.golang
	go mod tidy
