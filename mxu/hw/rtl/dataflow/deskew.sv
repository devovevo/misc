module deskew #(
    parameter int SIZE = 3,
    parameter int AWIDTH = 32
)(
    // Constrol signals
    input logic clk,
    input logic rst,

    // Data in - to be skewed
    input logic[(AWIDTH * SIZE) - 1 : 0] data_in,

    // Skewed data out
    output logic[(AWIDTH * SIZE - 1) : 0] deskewed_data_out
);
    logic [AWIDTH - 1 : 0] delay_regs[SIZE][SIZE];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int row = 0; row < SIZE; row++) begin
                for (int d = 0; d < SIZE; d++) begin
                    delay_regs[row][d] <= '0;
                end
            end
        end else begin
            for (int row = 1; row < SIZE; row++) begin
                delay_regs[row][0] <= data_in[row * AWIDTH +: AWIDTH];

                for (int d = 1; d < row; d++) begin
                    delay_regs[row][d] <= delay_regs[row][d - 1];
                end
            end
        end
    end

    assign deskewed_data_out[(SIZE - 1) * AWIDTH +: AWIDTH] =
        data_in[(SIZE - 1) * AWIDTH +: AWIDTH];
    genvar r;
    generate
        for (r = 0; r < SIZE - 1; r++) begin : gen_assign_out
            assign deskewed_data_out[r * AWIDTH +: AWIDTH] = delay_regs[r][SIZE - 2 - r];
        end
    endgenerate

endmodule
