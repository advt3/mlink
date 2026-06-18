terraform {
  required_version = ">= 1.0"
  required_providers {
    google = {
      source  = "hashicorp/google"
      version = "~> 5.0"
    }
    external = {
      source  = "hashicorp/external"
      version = "~> 2.0"
    }
  }
}

data "external" "gcloud_config" {
  program = ["bash", "-c", "token=$(gcloud auth print-access-token 2>/dev/null); project=$(gcloud config get-value project 2>/dev/null); if [ $? -eq 0 ] && [ -n \"$token\" ] && [ -n \"$project\" ]; then echo \"{\\\"token\\\": \\\"$token\\\", \\\"project\\\": \\\"$project\\\"}\"; else echo \"{\\\"error\\\": \\\"Not logged in to gcloud or no project set. Please run 'gcloud auth login' and set a project.\\\"}\" >&2; exit 1; fi"]
}

provider "google" {
  project      = data.external.gcloud_config.result.project
  region       = var.gcp_region
  zone         = var.gcp_zone
  access_token = data.external.gcloud_config.result.token
}
