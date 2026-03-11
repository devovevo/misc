import mxu_pkg::*;

module compute_fsm#(
   parameter int SIZE = 3,
   parameter int ADDR_WIDTH = 32
)(
    // Control pins
    input logic clk,
    input logic rst_n,

    // FIFO control
    input logic fifo_empty,
    input activation_compute_command_t fifo_out,
    output fifo_pop,

    // SRAM control
    output logic sram_cs,
    output logic [ADDR_WIDTH - 1 : 0] sram_addr,

    // Input
    input logic load_done_pulse,

    // Output
    output logic compute_done_pulse
);

endmodule


