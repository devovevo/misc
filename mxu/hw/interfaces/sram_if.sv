interface sram_if #(
    parameter int ADDR_WIDTH,
    parameter int DATA_WIDTH,
    parameter int READ_LATENCY
)(
    input logic clk,
    input logic rst_n
);
    // Interface signals
    logic   cs;
    logic   we;
    logic [ADDR_WIDTH - 1 : 0] addr;
    logic [DATA_WIDTH - 1 : 0] wdata;
    logic [DATA_WIDTH - 1 : 0] rdata;

    // Controller - CPU or DMA
    modport controller(
        input clk, rdata,
        output cs, we, addr, wdata
    );

    // Memory - Actual SRAM interface
    modport memory(
        input clk, cs, we, addr, wdata,
        output rdata
    );

endinterface
