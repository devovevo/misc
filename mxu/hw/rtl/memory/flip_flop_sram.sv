module flip_flop_sram#(
    parameter int DATA_WIDTH = 256,
    parameter int ADDR_WIDTH = 16,
    parameter int READ_LATENCY = 1
)(
    input logic clk,
    input logic rst_n,

    input logic cs_in,
    input logic [ADDR_WIDTH - 1 : 0 ] addr_in,
    input logic [DATA_WIDTH - 1 : 0 ] wdata_in,
    input logic we_in,

    output logic [DATA_WIDTH - 1 : 0] rdata_out
);
    logic [DATA_WIDTH - 1 : 0] mem [(1 << ADDR_WIDTH) - 1];
    logic [DATA_WIDTH - 1 : 0] rdata_pipeline[READ_LATENCY];

    assign rdata_out = rdata_pipeline[READ_LATENCY - 1];

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            rdata_pipeline <= '{default: '0};
        end else if (cs_in) begin
            if (we_in) begin
                mem[addr_in] <= wdata_in;
            end

            rdata_pipeline[0] <= mem[addr_in];
            for (int i = 1; i < READ_LATENCY; i++) begin
               rdata_pipeline[i] <= rdata_pipeline[i - 1];
            end
        end
    end
endmodule
