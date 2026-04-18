import mxu_pkg::*;

module load_unit_tb_top#(
    parameter int SIZE = 3,
    parameter int DATA_WIDTH = 16,
    parameter int ADDR_WIDTH = 16,
    parameter int READ_LATENCY = 3
)(
    input logic clk,
    input logic rst_n,

    input logic [ADDR_WIDTH - 1 : 0] dma_addr_in,
    input logic [DATA_WIDTH - 1 : 0] dma_wdata_in,
    input logic dma_we_in,
    input logic dma_done_pulse_in,

    input logic fifo_empty_in,
    input weight_load_cmd_t fifo_data_in,
    output logic fifo_pop_out,

    output logic [DATA_WIDTH - 1 : 0] top_shadow_out,
    output logic load_enable_out,
    output logic load_done_pulse_out
);
    logic sram_cs;
    logic [ADDR_WIDTH - 1 : 0] sram_addr;
    logic weight_valid;
    logic force_zero;

    load_fsm#(
        .SIZE(SIZE),
        .ADDR_WIDTH(ADDR_WIDTH),
        .READ_LATENCY(READ_LATENCY)
    ) load_fsm_inst (
        .clk(clk),
        .rst_n(rst_n),
        .dma_done_pulse_in(dma_done_pulse_in),
        .fifo_empty_in(fifo_empty_in),
        .fifo_data_in(fifo_data_in),
        .fifo_pop_out(fifo_pop_out),
        .sram_cs_out(sram_cs),
        .sram_addr_out(sram_addr),
        .weight_valid_out(weight_valid),
        .force_zero_out(force_zero),
        .load_done_pulse_out(load_done_pulse_out)
    );

    logic [ADDR_WIDTH - 1 : 0] true_sram_addr;
    always_comb begin
        if (dma_we_in) begin
            true_sram_addr = dma_addr_in;
        end else begin
            true_sram_addr = sram_addr;
        end
    end

    logic [DATA_WIDTH - 1 : 0] rdata;

    flip_flop_sram#(
        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH),
        .READ_LATENCY(READ_LATENCY)
    ) sram_inst (
        .clk(clk),
        .rst_n(rst_n),
        .cs_in(sram_cs),
        .addr_in(true_sram_addr),
        .wdata_in(dma_wdata_in),
        .we_in(dma_we_in),
        .rdata_out(rdata)
    );

    assign load_enable_out = weight_valid | force_zero;

    always_comb begin
        top_shadow_out = '0;
        if (weight_valid) begin
            top_shadow_out = rdata;
        end else if (force_zero) begin
            top_shadow_out = '0;
        end
    end

endmodule
