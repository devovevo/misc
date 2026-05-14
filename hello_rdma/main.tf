provider "google" {
  project = "hello-tofu"
  region  = "us-central1"
  zone    = "us-central1-a"
}

resource "google_compute_network" "main" {
  name                    = "rdma-net"
  auto_create_subnetworks = false
}

resource "google_compute_subnetwork" "us-east1" {
  name          = "rdma-us-east1-subnet"
  ip_cidr_range = "10.0.1.0/24"
  network       = google_compute_network.main.id
  region        = "us-east1"
}

resource "google_compute_firewall" "allow_rdma" {
  name    = "allow-rdma-internal"
  network = google_compute_network.main.id

  allow {
    protocol = "udp"
    ports    = ["4791"]
  }

  source_ranges = ["10.0.1.0/24"]
}

resource "google_service_account" "tester" {
  account_id   = "rdma-tester-sa"
  display_name = "SA for RDMA tester nodes"
}

resource "google_compute_instance" "tester" {
  for_each = var.node_names

  name         = "rdma-tester-${each.key}"
  machine_type = "e2-micro"



  boot_disk {
    initialize_params {
      image = "debian-cloud/debian-11"
    }
  }

  network_interface {
    network = "default"
  }

  service_account {
    email  = google_service_account.tester.email
    scopes = ["cloud-platform"]
  }
}
