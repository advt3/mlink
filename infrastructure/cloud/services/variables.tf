variable "gcp_region" {
  type        = string
  description = "The GCP Region"
  default     = "us-east1"
}

variable "gcp_zone" {
  type        = string
  description = "The GCP Zone"
  default     = "us-east1-c"
}

variable "gcp_machine_type" {
  type        = string
  description = "The GCP machine type (e.g., e2-micro for prod, e2-medium for provisioning)"
  default     = "e2-micro"
}
