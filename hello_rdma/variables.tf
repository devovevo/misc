variable "node_names" {
    description = "Names of the VMs to create"
    type = set(string)
    default = ["rdma-node-1", "rdma-node-2"]
}
