import mxu_pkg::*;

module compute_fsm#(
   parameter int SIZE = 3,
   parameter int ADDR_WIDTH = 32,
   parameter int READ_LATENCY = 3
)(
    // Control pins
    input logic clk,
    input logic rst_n,

    // FIFO control
    input logic fifo_empty_in,
    input activation_compute_command_t fifo_data_in,
    output logic fifo_pop_out,

    // SRAM control
    output logic sram_cs_out,
    output logic [ADDR_WIDTH - 1 : 0] sram_addr_out,

    // Input
    input logic load_done_pulse_in,

    // Output
    output logic compute_done_pulse_out
);
    typedef enum logic [2:0] { IDLE, POPPING, WAITING, LOADING, PADDING } state_t;
    state_t state, next_state;

    logic [ADDR_WIDTH - 1 : 0] cur_addr;


endmodule


