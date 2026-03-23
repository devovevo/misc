module flip_flop_sram#(
    parameter int DATA_WIDTH = 256,
    parameter int ADDR_WIDTH = 16,
    parameter int READ_LATENCY = 1
)(
    input logic clk,
    input logic rst_n,

    input logic cs,
    input logic [ADDR_WIDTH - 1 : 0 ] addr,
    input logic [DATA_WIDTH - 1 : 0 ] wdata,
    input logic we,

    output logic [DATA_WIDTH - 1 : 0] rdata
);
    logic [DATA_WIDTH - 1 : 0] mem [(1 << ADDR_WIDTH) - 1] /*verilator public*/
    logic [DATA_WIDTH - 1 : 0] rdata_pipeline[READ_LATENCY];

    assign rdata = rdata_pipeline[READ_LATENCY - 1];

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            rdata_pipeline <= '0;
        end else if (cs) begin
            if (we) begin
                mem[addr] <= wdata;
            end

            rdata_pipeline[0] <= mem[addr];
            for (int i = 1; i < READ_LATENCY; i++) begin
               rdata_pipeline[i] <= rdata_pipeline[i - 1];
            end
        end
    end
endmodule
