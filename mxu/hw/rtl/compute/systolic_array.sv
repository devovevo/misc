module systolic_array #(
    parameter int SIZE = 3,
    parameter int DWIDTH = 8,
    parameter int AWIDTH = 32
)(
    input  logic clock,
    input  logic rst,
    
    // Flattened inputs from the C++ Testbench
    input logic [(SIZE * DWIDTH) - 1 : 0] left_in,
    input logic [SIZE - 1 : 0] left_switch_in,
    input logic [(SIZE * AWIDTH) - 1 : 0] top_in,
    input logic [(SIZE * DWIDTH) - 1 : 0] top_shadow_in, // BUG 2 FIXED
    input logic [SIZE - 1 : 0] top_load_enable_in,
    
    // Flattened outputs to the C++ Testbench
    output logic [(SIZE * AWIDTH) - 1 : 0] bottom_out,
    output logic [(SIZE * DWIDTH) - 1 : 0] bottom_shadow_out
);

    logic [DWIDTH - 1 : 0] right_wire [SIZE][SIZE];
    logic right_switch_wire [SIZE][SIZE];
    logic [AWIDTH - 1 : 0] bottom_wire [SIZE][SIZE];
    logic [DWIDTH - 1 : 0] bottom_shadow_wire [SIZE][SIZE];
    
    genvar row, col;
    generate
        for (row = 0; row < SIZE; row++) begin : row_loop
            for (col = 0; col < SIZE; col++) begin : col_loop
                
                logic [DWIDTH - 1 : 0] pe_left_in;
                logic pe_left_switch_in;
                if (col == 0) begin : gen_left_edge
                    assign pe_left_in = left_in[row * DWIDTH +: DWIDTH];
                    assign pe_left_switch_in = left_switch_in[row];
                end else begin : gen_left_internal
                    assign pe_left_in = right_wire[row][col - 1];
                    assign pe_left_switch_in = right_switch_wire[row][col - 1]; // BUG 3 FIXED
                end

                logic [AWIDTH - 1 : 0] pe_top_in;
                logic [DWIDTH - 1 : 0] pe_top_shadow_in;
                if (row == 0) begin : gen_top_edge
                    assign pe_top_in = top_in[col * AWIDTH +: AWIDTH];
                    assign pe_top_shadow_in = top_shadow_in[col * DWIDTH +: DWIDTH];
                end else begin : gen_top_internal
                    assign pe_top_in = bottom_wire[row - 1][col];
                    assign pe_top_shadow_in = bottom_shadow_wire[row - 1][col];
                end

                logic pe_load_in;
                assign pe_load_in = top_load_enable_in[col]; // BUG 4 FIXED

                pe #(
                    .DWIDTH(DWIDTH),
                    .AWIDTH(AWIDTH)
                ) pe_inst (
                    .clock(clock),
                    .rst(rst),
                    
                    .left_in(pe_left_in),
                    .left_weight_switch_in(pe_left_switch_in),
                    .top_in(pe_top_in),
                    .top_shadow_in(pe_top_shadow_in),
                    .load_enable_in(pe_load_in),
                    
                    .right_out(right_wire[row][col]),
                    .right_weight_switch_out(right_switch_wire[row][col]), // BUG 5 FIXED
                    .bottom_out(bottom_wire[row][col]),
                    .bottom_shadow_out(bottom_shadow_wire[row][col])
                );
            end
        end
    endgenerate

    generate
        for (col = 0; col < SIZE; col++) begin : output_loop
            assign bottom_out[col * AWIDTH +: AWIDTH] = bottom_wire[SIZE - 1][col];
            assign bottom_shadow_out[col * DWIDTH +: DWIDTH] = bottom_shadow_wire[SIZE - 1][col];
        end
    endgenerate

endmodule
