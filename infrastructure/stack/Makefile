export MQTT_DOMAIN ?= localhost
export LE_EMAIL ?= admin@localhost
export MQTT_USER ?= admin
export MQTT_PASS ?= admin

.PHONY: local-up
local-up:
	docker compose -f docker-compose.yml -f docker-compose.local.yml up -d

.PHONY: local-down
local-down:
	docker compose -f docker-compose.yml -f docker-compose.local.yml down
