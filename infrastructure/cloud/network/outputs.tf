output "network_name" {
  value = google_compute_network.mqtt_vpc.name
}

output "subnet_name" {
  value = google_compute_subnetwork.mqtt_subnet.name
}

output "static_ip_address" {
  value = google_compute_address.mqtt_ip.address
}
