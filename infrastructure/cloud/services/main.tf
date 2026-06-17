# --- Secrets Section ---
resource "google_secret_manager_secret" "mqtt_username" {
  secret_id = "metric-link-secret-mqtt-username"
  replication {
    auto {}
  }
}

resource "google_secret_manager_secret" "mqtt_password" {
  secret_id = "metric-link-secret-mqtt-password"
  replication {
    auto {}
  }
}

resource "google_secret_manager_secret" "telegram_bot_token" {
  secret_id = "metric-link-secret-telegram-bot-token"
  replication {
    auto {}
  }
}

# --- Network Data Lookups ---
data "google_compute_network" "mqtt_vpc" {
  name = "metric-link-vpc"
}

data "google_compute_subnetwork" "mqtt_subnet" {
  name   = "metric-link-subnet-us-east1"
  region = var.gcp_region
}

data "google_compute_address" "mqtt_ip" {
  name   = "metric-link-ip-us-east1-mqtt"
  region = var.gcp_region
}

# --- SSH Key & Username Lookup ---
data "external" "ssh_key" {
  program = ["bash", "-c", "key_path=\"$HOME/.ssh/google_compute_engine.pub\"; username=\"$(id -un)\"; if [ -f \"$key_path\" ]; then echo \"{\\\"key\\\": \\\"$(cat \"$key_path\")\\\", \\\"username\\\": \\\"$username\\\"}\"; else echo \"{\\\"key\\\": \\\"\\\", \\\"username\\\": \\\"$username\\\"}\"; fi"]
}

# --- Compute Instance ---
resource "google_compute_instance" "mqtt_vm" {
  name                      = "metric-link-vm-us-east1-mqtt"
  machine_type              = var.gcp_machine_type
  zone                      = var.gcp_zone
  allow_stopping_for_update = true
  tags                      = ["metric-link-mqtt-broker"]

  # Apply Google best practice labels for environment and project tagging
  labels = {
    env     = "prod"
    project = data.external.gcloud_config.result.project
  }

  boot_disk {
    initialize_params {
      image = "debian-cloud/debian-12"
      size  = 30
      type  = "pd-standard"
    }
  }

  network_interface {
    network    = data.google_compute_network.mqtt_vpc.id
    subnetwork = data.google_compute_subnetwork.mqtt_subnet.id

    access_config {
      nat_ip       = data.google_compute_address.mqtt_ip.address
      network_tier = "STANDARD"
    }
  }

  metadata = {
    ssh-keys       = data.external.ssh_key.result.key != "" ? "${data.external.ssh_key.result.username}:${data.external.ssh_key.result.key}" : null
    startup-script = <<-EOT
      #!/bin/bash
      # Configure 512MB Swap
      if [ ! -f /swapfile ]; then
        fallocate -l 512M /swapfile
        chmod 600 /swapfile
        mkswap /swapfile
        swapon /swapfile
        echo '/swapfile none swap sw 0 0' >> /etc/fstab
      fi

      apt-get update
      apt-get install -y apt-transport-https ca-certificates curl gnupg lsb-release docker.io docker-compose rsync
      systemctl enable docker
      systemctl start docker
    EOT
  }
}
