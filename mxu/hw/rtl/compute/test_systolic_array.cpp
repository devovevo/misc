#include <gtest/gtest.h>
#include <Eigen/Dense>     
#include "hw/shared/verilator_pins.h"
#include "hw/shared/verilator_test_fixture.h"
#include "Vsystolic_array.h"

using SystolicArrayTest = VerilatorTestFixture<Vsystolic_array>;

TEST_F(SystolicArrayTest, RandomMatrixMultiplyLifecycle) {
    start_tracing("systolic_array_rand_matmul_test.fst");

    const int DATA_WIDTH = 8;
    int max_val = (1 << DATA_WIDTH) - 1;
    
    Eigen::MatrixXi A = Eigen::MatrixXi::Random(SIZE, SIZE).cwiseAbs().unaryExpr(
        [max_val](int x) { return x % max_val; }
    );
    Eigen::MatrixXi B = Eigen::MatrixXi::Random(SIZE, SIZE).cwiseAbs().unaryExpr(
        [max_val](int x) { return x % max_val; }
    );
    Eigen::MatrixXi C_expected = A * B; 

    dut->rst_n = 0;
    tick(dut->clk);
    dut->rst_n = 1;
    tick(dut->clk);
    
    // Phase 1: Load Weights
    for (int i = 0; i < SIZE; i++) {
        std::vector<uint32_t> row_to_load(SIZE);
        for(int col = 0; col < SIZE; col++) {
            row_to_load[col] = B(SIZE - i - 1, col); 
        }
        set_pin(dut->top_shadow_in, row_to_load, DATA_WIDTH);
        set_pin(dut->top_load_enable_in, std::vector<uint32_t>(SIZE, 1), 1); 
        tick(dut->clk);
    }
    set_pin(dut->top_load_enable_in, std::vector<uint32_t>(SIZE, 0), 1);
    tick(dut->clk);

    // Phase 2: Stream Activations & Catch Outputs
    // Add 1 extra cycle to account for the new data delay pipeline
    int TOTAL_CYCLES = 3 * SIZE + 1; 
    Eigen::MatrixXi deskewed_C = Eigen::MatrixXi::Zero(SIZE, SIZE);

    for (int t = 0; t < TOTAL_CYCLES; t++) {
        std::vector<uint32_t> left_in_array(SIZE, 0);
        std::vector<uint32_t> switch_in_array(SIZE, 0);

        for (int r = 0; r < SIZE; r++) {
            // Trigger the weight switch to load from the shadow register
            if (t == r) switch_in_array[r] = 1; 
            
            // FIXED: Data must arrive 1 cycle AFTER the switch triggers!
            int i = t - r - 1; 
            if (i >= 0 && i < SIZE) left_in_array[r] = A(i, r);
        }

        set_pin(dut->left_in, left_in_array, DATA_WIDTH);
        set_pin(dut->left_switch_in, switch_in_array, 1);
        tick(dut->clk); 
        
        std::vector<uint32_t> out_array = get_pin(dut->bottom_out, SIZE, 32);
        
        for (int c = 0; c < SIZE; c++) {
            // Deskew logic must also shift by 1 to accommodate the data delay
            int i = (t + 1) - SIZE - c - 1; 
            if (i >= 0 && i < SIZE) {
                deskewed_C(i, c) = out_array[c]; 
            }
        }
    }

    // Phase 3: Verification
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            EXPECT_EQ(deskewed_C(r, c), C_expected(r, c)) 
                << "Mismatch at row " << r << ", col " << c 
                << ". Expected: " << C_expected(r, c) << " Got: " << deskewed_C(r, c);
        }
    }
}
