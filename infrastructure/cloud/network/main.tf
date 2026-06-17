# 1. Custom VPC Network
resource "google_compute_network" "mqtt_vpc" {
  name                    = "metric-link-vpc"
  auto_create_subnetworks = false
}

# 2. Regional Subnet
resource "google_compute_subnetwork" "mqtt_subnet" {
  name          = "metric-link-subnet-us-east1"
  network       = google_compute_network.mqtt_vpc.id
  region        = var.gcp_region
  ip_cidr_range = "10.0.1.0/24"
}

# 3. Firewall Rule for MQTT & Traefik ACME
resource "google_compute_firewall" "mqtt_ingress" {
  name    = "metric-link-firewall-allow-mqtt"
  network = google_compute_network.mqtt_vpc.name

  allow {
    protocol = "tcp"
    ports    = ["80", "8883"]
  }

  source_ranges = ["0.0.0.0/0"]
  target_tags   = ["metric-link-mqtt-broker"]
}

# 4. Static External IP (Standard Tier)
resource "google_compute_address" "mqtt_ip" {
  name         = "metric-link-ip-us-east1-mqtt"
  region       = var.gcp_region
  network_tier = "STANDARD"
}

# 5. Firewall Rule for SSH
resource "google_compute_firewall" "ssh_ingress" {
  name    = "metric-link-firewall-allow-ssh"
  network = google_compute_network.mqtt_vpc.name

  allow {
    protocol = "tcp"
    ports    = ["22"]
  }

  source_ranges = var.ssh_source_ranges
}
