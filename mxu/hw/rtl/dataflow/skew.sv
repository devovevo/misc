module skew #(
    parameter int SIZE = 3,
    parameter int AWIDTH = 32
)(
    // Constrol signals
    input logic clk,
    input logic rst,

    // Data in - to be skewed
    input logic[(AWIDTH * SIZE) - 1 : 0] data_in,
    input logic[SIZE - 1 : 0] switch_in,

    // Skewed data out
    output logic[(AWIDTH * SIZE - 1) : 0] skewed_data_out,
    output logic[SIZE - 1 : 0] skewed_switch_out
);
    logic [AWIDTH - 1 : 0] data_delay_regs[SIZE][SIZE];
    logic switch_delay_regs[SIZE][SIZE];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int row = 0; row < SIZE; row++) begin
                for (int d = 0; d < SIZE; d++) begin
                    data_delay_regs[row][d] <= '0;
                    switch_delay_regs[row][d] <= 0;
                end
            end
        end else begin
            for (int row = 1; row < SIZE; row++) begin
                data_delay_regs[row][0] <= data_in[row * AWIDTH +: AWIDTH];
                switch_delay_regs[row][0] <= switch_in[row];

                for (int d = 1; d < row; d++) begin
                    data_delay_regs[row][d] <= data_delay_regs[row][d - 1];
                    switch_delay_regs[row][d] <= switch_delay_regs[row][d - 1];
                end
            end
        end
    end

    assign skewed_data_out[0 * AWIDTH +: AWIDTH] = data_in[0 * AWIDTH +: AWIDTH];
    genvar r;
    generate
        for (r = 1; r < SIZE; r++) begin : gen_assign_out
            assign skewed_data_out[r * AWIDTH +: AWIDTH] = data_delay_regs[r][r - 1];
            assign skewed_switch_out[r] = switch_delay_regs[r][r - 1];
        end
    endgenerate

endmodule
