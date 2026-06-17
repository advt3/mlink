# Infrastructure & Operations Guide

Deployment, credential management, TLS setup, and alerting configuration for local container stack testing and GCP cloud provisioning.

---

## 1. Local Testing (Unencrypted)

To spin up the local observability and broker services (which exposes Mosquitto's unencrypted port `1883` directly on your localhost, keeping local testing simple and TLS-free):

```bash
cd infrastructure/stack

# Start the stack
make local-up

# Stop the stack
make local-down
```

### Monitoring Dashboards & UIs
* **vmalert UI**: `http://localhost:8880` (Alert execution status)
* **Alertmanager UI**: `http://localhost:9093` (Active notifications & silences)
* **VictoriaMetrics UI**: `http://localhost:8428` (Metric querying)

---

## 2. 1Password Secret Management

We use the 1Password `op` CLI to retrieve credentials and TLS certificates without committing secrets to Git. This ensures that passwords, keys, and certificates are kept completely out of version control and can be safely retrieved on different machines.

Make sure you are logged in to the `op` CLI before running these targets.

### 1. MQTT Credentials
Retrieve user/pass configurations and generate a hashed Mosquitto password file inside the git-ignored `.persistence/` directory:
```bash
cd infrastructure/stack

# Generates the password_file in ../.persistence/password_file using Eclipse-Mosquitto passwd utility
make hash-passwd
```

### 2. TLS Server Certificates (Optional)
If deploying custom broker certificates (e.g. self-signed or pre-issued), retrieve the CA certificate/key from 1Password into the local `mosquitto/config/certs` folder (which is git-ignored):
```bash
cd infrastructure/stack
make get-certs
```

### 3. Telegram Alertmanager Config
Retrieve the credentials for the Telegram integration and write the token file to the git-ignored `.persistence` path:
```bash
# Write Telegram token to git-ignored configuration path
op read "op://Automation/environment-measures-telegram/credential" > ../.persistence/telegram_token
```

---

## 3. Firmware CA Integration (Pico W / ESP32)

To compile the trusted Let's Encrypt Root CA certificate into the firmware:

1. **Download and format the Root CA / Server Certificate**:
   - **Method A: Let's Encrypt Root CA (Default)**
     This downloads the Let's Encrypt `ISRG Root X1` certificate PEM and formats it as a C-string header in `device/firmware/src/ca_cert.inc`:
     ```bash
     cd infrastructure/stack
     make get-root-ca
     ```
   - **Method B: Extract directly from the live broker**
     If you need to fetch and format the exact certificate chain/leaf from the running broker directly (e.g. for testing custom or self-signed certs), run this `openssl` command from the repository root:
     ```bash
     openssl s_client -showcerts -connect <YOUR_BROKER_DOMAIN>:8883 </dev/null 2>/dev/null | openssl x509 -outform PEM | awk '{print "\"" $0 "\\n\""}' > device/firmware/src/ca_cert.inc
     ```
2. **Configure local overrides**:
   Copy `device/firmware/local.conf.example` to `device/firmware/local.conf` and specify the custom broker IP/Host and credentials (this file is ignored by Git).
3. **Build the firmware**:
   Using `west` as usual (e.g., `make build-pico` or `make build-esp32`). The firmware will load the root CA to trust the broker TLS connection when connecting to your domain on port `8883`.

---

## 4. Remote GCP Infrastructure Provisioning

The GCP deployment configuration is located under `infrastructure/cloud`.

### 1. Provision the Network Layer
This provisions the custom VPC network, subnetwork, firewall rules, and static IP.
```bash
cd infrastructure/cloud/network
terraform init
terraform plan
terraform apply
```

### 2. Provision the Services Layer
This provisions the GCE VM instance (which auto-installs Docker and Docker Compose via startup-script) and Secret Manager secret definitions.
```bash
cd ../services
terraform init
terraform plan
terraform apply
```

---

## 5. VM Service Deployment

After provisioning GCP resources, deploy the cloud container stack using rsync over SSH:

```bash
cd infrastructure/stack

# Deploy configuration and restart the container services on the remote VM
make deploy
```

*This command automatically retrieves your username, password, and public domain name from 1Password, updates the remote configuration environment, uploads the Mosquitto password file to `~/.persistence/password_file`, and boots the containers with Traefik requesting/renewing a Let's Encrypt SSL/TLS certificate automatically on port `8883`.*

### Accessing Dashboards on GCP via SSH Tunneling

To securely access the private Prometheus/VictoriaMetrics and Alertmanager dashboards on the remote GCE VM without exposing their ports to the public internet or running heavy network agents like Tailscale, use SSH port forwarding.

Run this command on your local machine:
```bash
ssh -i ~/.ssh/google_compute_engine -N \
  -L 8428:localhost:8428 \
  -L 8880:localhost:8880 \
  -L 9093:localhost:9093 \
  $(id -un)@<YOUR_VM_IP_OR_DOMAIN>
```

Once established, open the UIs in your local web browser:
* **VictoriaMetrics UI**: [http://localhost:8428/vmui](http://localhost:8428/vmui)
* **vmalert UI**: [http://localhost:8880](http://localhost:8880)
* **Alertmanager UI**: [http://localhost:9093](http://localhost:9093)

---

## 6. Testing the Cloud Broker Connection

Since the cloud broker uses Let's Encrypt TLS certificates, standard clients automatically trust the connection.

### Retrieve Credentials from 1Password
You can read the username, password, and domain name dynamically using the 1Password CLI:
```bash
export MQTT_USER=$(op read "op://Automation/environment-measures-mqtt-auth/username")
export MQTT_PASS=$(op read "op://Automation/environment-measures-mqtt-auth/password")
export MQTT_DOMAIN=$(op read "op://Automation/environment-measures-mqtt-auth/domain")
```

### Using MQTTX (EMQX CLI)

**Subscribe to a topic:**
```bash
mqttx sub -t "sensors/environment" \
  -h "$MQTT_DOMAIN" \
  -p 8883 \
  -u "$MQTT_USER" \
  -P "$MQTT_PASS" \
  --protocol mqtts
```

**Publish a payload:**
```bash
mqttx pub -t "sensors/environment" \
  -m '{"temperature": 23.4, "humidity": 45.2, "pressure": 1012.3}' \
  -h "$MQTT_DOMAIN" \
  -p 8883 \
  -u "$MQTT_USER" \
  -P "$MQTT_PASS" \
  --protocol mqtts
```
