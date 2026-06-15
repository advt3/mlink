# Infrastructure Stack

This folder contains the Docker Compose configurations to run the environment observability and telemetry stack.

## Local Quick Start

To spin up the stack locally (which exposes Mosquitto broker on port `1883` and VictoriaMetrics on `8428`):

```bash
# Start the stack
make local-up

# Stop the stack
make local-down
```

## Configuration & Persistence

All persistent data and database files reside in the gitignored directory:
`../.persistence/` (relative to this folder). No secret files or tokens are required for this version.
