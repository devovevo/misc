#include <iostream>
#include <iomanip>
#include "Vsystolic_array.h"
#include "verilated.h"

double sc_time_stamp() { return 0; }

#define SIZE 3
#define DWIDTH 8
#define AWIDTH 32

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vsystolic_array* dut = new Vsystolic_array;

    // Weight Matrix (3x3)
    int weights[SIZE][SIZE] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };

    // Pure Activation Matrix (3x3) - NO PADDING!
    int activations[SIZE][SIZE] = {
        {10, 10, 10},
        {20, 20, 20},
        {30, 30, 30}
    };

    // Output Matrix (3x3)
    int outputs[SIZE][SIZE] = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    int expected_outputs[SIZE][SIZE];
    // Compute expected outputs
    for (int i=0; i<SIZE; i++) {
        for (int j=0; j<SIZE; j++) {
            expected_outputs[i][j] = 0;
            for (int k=0; k<SIZE; k++) {
                expected_outputs[i][j] += weights[i][k] * activations[k][j];
            }
        }
    }

    // Initialize
    dut->clock = 0; dut->rst = 1;
    dut->left_in = 0; dut->left_switch_in = 0;
    dut->top_shadow_in = 0; dut->top_load_enable_in = 0;
    for(int i=0; i<SIZE; i++) dut->top_in[i] = 0;
    
    dut->eval(); dut->clock = 1; dut->eval();
    dut->clock = 0; dut->rst = 0; dut->eval();

    std::cout << "--- STARTING SIMULATION ---" << std::endl;
    int cycle = 0;
    
    // --- PHASE 1: POPULATION (The Shadow Shift Register) ---
    std::cout << "\n[Phase 1: Loading Shadow Weights]" << std::endl;
    for (int load_step = 0; load_step < SIZE; load_step++) {
        dut->clock = 0;
        dut->top_load_enable_in = 0x7; // Broadcast to all columns
        
        uint32_t packed_shadow = 0;
        int row_to_load = (SIZE - 1) - load_step;
        for (int col = 0; col < SIZE; col++) {
            packed_shadow |= (weights[row_to_load][col] << (col * DWIDTH));
        }
        dut->top_shadow_in = packed_shadow;

        dut->eval(); dut->clock = 1; dut->eval();
        std::cout << "Cycle " << cycle << ": Loaded shadow row " << row_to_load << std::endl;
        cycle++;
    }

    // --- PHASE 2: EXECUTION (Dynamic Skewing + Control) ---
    std::cout << "\n[Phase 2: Swapping Weights & Computing]" << std::endl;
    dut->top_load_enable_in = 0;
    dut->top_shadow_in = 0;

    // Run enough cycles to drain the pipeline
    for (int compute_step = 0; compute_step < (SIZE * SIZE); compute_step++) {
        dut->clock = 0;
        uint32_t packed_left_in = 0;
        uint8_t packed_switch = 0;
        
        for (int row = 0; row < SIZE; row++) {
            // We offset the active_col by -1 to dynamically create the 1-cycle bubble
            int active_col = compute_step - row - 1; 
            uint32_t val = 0;
            
            // The exact cycle BEFORE the data starts is the switch cycle for this row
            if (compute_step == row) {
                packed_switch |= (1 << row); 
                // val remains 0 here, feeding the physical bubble into the pin
            }
            
            // If we are past the switch cycle and within data bounds, fetch the data
            if (active_col >= 0 && active_col < SIZE) {
                val = activations[active_col][row];
            }
            
            packed_left_in |= (val << (row * DWIDTH));
        }
        
        dut->left_in = packed_left_in;
        dut->left_switch_in = packed_switch;

        dut->eval(); dut->clock = 1; dut->eval();

        std::cout << "Cycle " << std::setw(2) << cycle << " | Switch Signal: " << (int)dut->left_switch_in << " | Bottom Outputs: [ ";
        for (int col = 0; col < SIZE; col++) {
            std::cout << std::setw(4) << dut->bottom_out[col] << " ";
        }
        std::cout << "]" << std::endl;
        cycle++;
    }

    delete dut;
    std::cout << "\n--- SIMULATION COMPLETE ---" << std::endl;
    return 0;
}