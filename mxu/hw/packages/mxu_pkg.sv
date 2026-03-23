package mxu_pkg;
    typedef struct packed {
        logic [6:0] reserved;
        logic wait_for_dma;
        logic [7:0] row_count;
        logic [15:0] base_addr;
    } weight_load_cmd_t;

    typedef struct packed {
        logic [6:0] reserved;
        logic wait_for_load;
        logic [7:0] row_count;
        logic [15:0] base_addr;
    } activation_compute_cmd_t;
endpackage;
