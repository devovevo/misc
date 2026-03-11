module pe #(
    parameter int DATA_WIDTH = 8,
    parameter int ACC_WIDTH = 32
)(
    input logic clock,
    input logic rst,

    input logic [DATA_WIDTH - 1 : 0] left_in,
    input logic [ACC_WIDTH - 1 : 0] top_in,
    input logic [DATA_WIDTH - 1 : 0] top_shadow_in,
    input logic load_enable_in,
    input logic left_weight_switch_in, // BUG 1 FIXED

    output logic [DATA_WIDTH - 1 : 0] right_out,
    output logic [ACC_WIDTH - 1 : 0] bottom_out,
    output logic [DATA_WIDTH - 1 : 0] bottom_shadow_out,
    output logic right_weight_switch_out
);

    logic [DATA_WIDTH - 1 : 0] weight;
    logic [DATA_WIDTH - 1 : 0] shadow_weight;
    logic [DATA_WIDTH - 1 : 0] value;
    logic [ACC_WIDTH - 1 : 0] result;
    logic right_weight_switch;

    assign right_out = value;
    assign bottom_out = result;
    assign bottom_shadow_out = shadow_weight;
    assign right_weight_switch_out = right_weight_switch;

    always_ff @(posedge clock) begin
        if (rst) begin
            weight <= '0;
            shadow_weight <= '0;
            value <= '0;
            result <= '0;
            right_weight_switch <= 1'b0;
        end else begin
            // 1. Pipeline data and control horizontally
            value <= left_in;
            right_weight_switch <= left_weight_switch_in;

            // 2. Load shadow
            if (load_enable_in) begin
                shadow_weight <= top_shadow_in;
            end

            // 3. Switch weight
            if (left_weight_switch_in) begin
                weight <= shadow_weight;
            end

            // 4. Compute result (The 1-Cycle Bubble architecture!)
            result <= (weight * left_in) + top_in;
        end
    end

endmodule
