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
  name                     = "rdma-us-east1-subnet"
  ip_cidr_range            = "10.0.1.0/24"
  network                  = google_compute_network.main.id
  region                   = "us-east1"
  private_ip_google_access = true
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

resource "google_compute_firewall" "allow_iap_ssh" {
  name    = "allow-iap-ssh-google"
  network = google_compute_network.main.id

  allow {
    protocol = "tcp"
    ports    = ["22"]
  }

  source_ranges = ["35.235.240.0/20"]
}

resource "google_compute_router" "router" {
  name    = "rdma-net-router-us-east1"
  region  = google_compute_subnetwork.us-east1.region
  network = google_compute_network.main.id
}

resource "google_compute_router_nat" "nat" {
  name                               = "rdma-net-nat-us-east1"
  router                             = google_compute_router.router.name
  region                             = google_compute_router.router.region
  nat_ip_allocate_option             = "AUTO_ONLY"
  source_subnetwork_ip_ranges_to_nat = "ALL_SUBNETWORKS_ALL_IP_RANGES"
}

resource "google_service_account" "tester" {
  account_id   = "rdma-tester-sa"
  display_name = "SA for RDMA tester nodes"
}

resource "google_compute_instance" "tester" {
  for_each = var.node_names

  name         = "rdma-tester-${each.key}"
  machine_type = "e2-micro"

  zone = "us-east1-b"

  labels = {
    role = "rdma-tester"
  }

  boot_disk {
    initialize_params {
      image = "ubuntu-os-cloud/ubuntu-2404-lts-amd64"
    }
  }

  network_interface {
    network    = google_compute_network.main.id
    subnetwork = google_compute_subnetwork.us-east1.id
  }

  service_account {
    email  = google_service_account.tester.email
    scopes = ["cloud-platform"]
  }

  allow_stopping_for_update = true
}
