import mxu_pkg::*;

module load_fsm #(
    parameter SIZE = 3,
    parameter ADDR_WIDTH = 32,
    parameter READ_LATENCY = 1
)(
    // Control pin
    input logic clk,
    input logic rst_n,

    // Fifo control
    input logic fifo_empty,
    input weight_load_cmd_t fifo_data_out,
    output logic fifo_pop,

    // DMA control signal
    input logic dma_done_pulse,

    // SRAM driver
    output logic sram_cs,
    output logic [ADDR_WIDTH - 1 : 0] sram_addr,

    // Systolic array driver
    output logic weight_valid,
    output logic force_zero,
    output logic load_done_pulse
);
    typedef enum logic [2:0] {IDLE, POPPING, WAITING, LOADING, PADDING} state_t;
    state_t state, next_state;

    logic [ADDR_WIDTH - 1: 0] current_addr;
    logic [7:0] sram_rows_remaining;
    logic [7:0] pad_rows_remaining;

    logic fsm_shift_valid;
    logic fsm_force_zero;
    logic fsm_load_done;

    logic [READ_LATENCY - 1 : 0] valid_pipeline;
    logic [READ_LATENCY - 1 : 0] zero_pipeline;
    logic [READ_LATENCY - 1 : 0] done_pipeline;

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            valid_pipeline <= '0;
            zero_pipeline <= '0;
            done_pipeline <= '0;
        end else begin
            valid_pipeline <= {valid_pipeline[READ_LATENCY - 2 : 0], fsm_shift_valid};
            zero_pipeline <= {zero_pipeline[READ_LATENCY - 2 : 0], fsm_force_zero};
            done_pipeline <= {done_pipeline[READ_LATENCY - 2 : 0], fsm_load_done};
        end
    end

    assign weight_valid = valid_pipeline[READ_LATENCY - 1];
    assign force_zero = zero_pipeline[READ_LATENCY - 1];
    assign load_done_pulse = done_pipeline[READ_LATENCY - 1];

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            state <= IDLE;
            current_addr <= '0;
            sram_rows_remaining <= '0;
            pad_rows_remaining <= '0;
        end else begin
            state <= next_state;
            if (state == POPPING) begin
                current_addr <= fifo_data_out.base_addr[AWIDTH - 1 : 0] + fifo_data_out.row_count - 1;
                sram_rows_remaining <= fifo_data_out.row_count;
                pad_rows_remaining <= SIZE - fifo_data_out.row_count;
            end else if (state == LOADING) begin
                current_addr <= current_addr - 1;
                sram_rows_remaining <= rows_remaining - 1;
            end else if (state == PADDING) begin
                pad_rows_remaining <= pad_rows_remaining - 1;
            end
        end
    end

    always_comb begin
        next_state = state;
        fifo_pop = 1'b0;
        sram_cs = 1'b0;
        sram_addr = current_addr;
        fsm_shift_valid = 1'b0;
        fsm_force_zero = 1'b0;
        fsm_load_done = 1'b0;

        unique case (state)
            IDLE: begin
                if (!fifo_empty) begin
                    fifo_pop = 1'b1;
                    next_state = POPPING;
                end
            end

            POPPING: begin
                if (fifo_data_out.wait_for_dma && !dma_done_pulse)
                    next_state = WAITING;
                else if (fifo_data_out.row_count > 0) begin
                    next_state = LOADING;
                end else begin
                    next_state = PADDING;
                end
            end

            WAITING: begin
                if (dma_done_pulse) begin
                    if (sram_rows_remaining > 0) begin
                        next_state = LOADING;
                    end else begin
                        next_state = PADDING;
                    end
                end else begin
                    next_state = WAITING;
                end

            LOADING: begin
                sram_cs = 1'b1;
                fsm_shift_valid = 1'b1;

                if (sram_rows_remaining == 1) begin
                    if (pad_rows_remaining > 0) begin
                        next_state = PADDING;
                    end else begin
                        next_state = IDLE;
                        fsm_load_done = 1'b1;
                    end
                end
            end

            PADDING: begin
                fsm_force_zero = 1'b1;
                if (pad_rows_remaining == 1) begin
                    next_state = IDLE;
                    fsm_load_done = 1'b1;
                end
            end
        endcase
    end
endmodule
