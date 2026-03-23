module load_unit_tb_top#(
    parameter int DATA_WIDTH = 16,
    parameter int ADDR_WIDTH = 16,
    parameter int READ_LATENCY = 3
)(
    input logic clk,
    input logic rst_n,

    input logic dma_addr_in,
    input logic dma_wdata_in,
    input logic dma_we_in,
    input logic dma_done_pulse,

    input logic fifo_empty,
    input weight_load_cmt_d fifo_data_out,
    output logic fifo_pop,

    output logic [DATA_WIDTH - 1 : 0] top_shadow,
    output logic load_enable,
    output logic load_done_pulse
);
    logic sram_cs;
    logic [ADDR_WIDTH - 1 : 0] sram_addr;
    logic weight_valid;
    logic force_zero;

    load_fsm#(
        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH),
        .READ_LATENCY(READ_LATENCY)
    )(
        .clk(clk),
        .rst_n(rst_n),
        .dma_done_pulse(dma_done_pulse),
        .fifo_empty(fifo_empty),
        .fifo_data_out(fifo_data_out),
        .fifo_pop(fifo_pop),
        .sram_cs(sram_cs),
        .sram_addr(sram_addr),
        .weight_valid(weight_valid),
        .force_zero(force_zero),
        .load_done_pulse(load_done_pulse)
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
    )(
        .clk(clk),
        .rst_n(rst_n),
        .cs(sram_cs),
        .addr(true_sram_addr),
        .wdata(dma_wdata_in),
        .we(dma_we_in),
        .rdata(rdata)
    );

    assign load_enable = weight_valid | force_zero;

    always_comb begin
        if (weight_valid) begin
            top_shadow = rdata;
        end else if (force_zero) begin
            top_shadow = '0;
        end
    end

endmodule
