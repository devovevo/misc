provider "google" {
  region  = "us-east1"
  zone    = "us-east1-b"
}

resource "google_compute_network" "k8s_vpc" {
  name                    = "prod-k8s-vpc"
  auto_create_subnetworks = false
}

resource "google_compute_subnetwork" "k8s_subnet" {
  name          = "prod-k8s-subnet"
  ip_cidr_range = "10.10.0.0/24"
  network       = google_compute_network.k8s_vpc.id
  region        = "us-east1"
}

resource "google_compute_firewall" "k8s_allow_internal" {
  name    = "prod-k8s-internal"
  network = google_compute_network.k8s_vpc.id
  allow { protocol = "4" }
  allow { protocol = "tcp" }
  allow { protocol = "udp" }
  allow { protocol = "icmp" }
  source_ranges = ["10.10.0.0/24"]
}

resource "google_compute_firewall" "k8s_allow_external" {
  name    = "prod-k8s-external"
  network = google_compute_network.k8s_vpc.id
  allow {
    protocol = "tcp"
    ports    = ["22", "6443"]
  }
  source_ranges = ["0.0.0.0/0"]
}

resource "google_compute_instance" "control_plane" {
  name         = "control-plane-1"
  machine_type = "e2-medium"
  zone         = "us-east1-b"
  
  # THE MAGIC TAG FOR ANSIBLE
  labels = {
    role = "k8s-master"
  }

  can_ip_forward = true

  boot_disk {
    initialize_params {
      image = "ubuntu-os-cloud/ubuntu-2404-lts-amd64"
      size  = 20
    }
  }
  network_interface {
    network    = google_compute_network.k8s_vpc.id
    subnetwork = google_compute_subnetwork.k8s_subnet.id
    access_config {} 
  }
  /*
  scheduling {
    provisioning_model = "SPOT"
    on_host_maintenance = "TERMINATE"
    preemptible = true
    automatic_restart = false
  }
  */
}

resource "google_compute_instance" "workers" {
  count        = 2
  name         = "worker-${count.index + 1}"
  machine_type = "e2-medium"
  zone         = "us-east1-b"
  
  # THE MAGIC TAG FOR ANSIBLE
  labels = {
    role = "k8s-worker"
  }

  can_ip_forward = true

  boot_disk {
    initialize_params {
      image = "ubuntu-os-cloud/ubuntu-2404-lts-amd64"
      size  = 20
    }
  }
  network_interface {
    network    = google_compute_network.k8s_vpc.id
    subnetwork = google_compute_subnetwork.k8s_subnet.id
    access_config {} 
  }
  /*
  scheduling {
    provisioning_model = "SPOT"
    preemptible = true
    automatic_restart = false
    on_host_maintenance = "TERMINATE"
  }
  */
}
