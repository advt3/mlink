variable "gcp_region" {
  type        = string
  description = "The GCP Region"
  default     = "us-east1"
}

variable "gcp_zone" {
  type        = string
  description = "The GCP Zone"
  default     = "us-east1-b"
}

variable "ssh_source_ranges" {
  type        = list(string)
  description = "Allowed CIDR ranges for SSH access. Defaults to Google IAP range (35.235.240.0/20)."
  default     = ["35.235.240.0/20"]
}
