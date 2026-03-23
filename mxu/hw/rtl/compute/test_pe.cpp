#include <random>
#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "hw/shared/verilator_pins.h"
#include "hw/shared/verilator_test_fixture.h"
#include "Vpe.h"

using PETest = VerilatorTestFixture<Vpe>;

TEST_F(PETest, RandomLoadLifecycle) {
    start_tracing("pe_rand_load.fst");

    const int DATA_WIDTH = 8;
    int max_value = (1 << DATA_WIDTH) - 1;

    dut->rst_n = 0;
    tick(dut->clk);
    dut->rst_n = 1;
    tick(dut->clk);
    
    vector<uint32_t> load_enable_in = {1};
    set_pin(dut->load_enable_in, load_enable_in, DATA_WIDTH);

    random_device rd;

    mt19937 gen(rd());
    uniform_int_distribution<int> distrib(0, max_value);
 
    const int NUM_TESTS = 100;
    for (int i = 0; i < NUM_TESTS; i++) {
        unsigned int rand_int = distrib(gen);
        vector<uint32_t> top_shadow_in = {rand_int};

        set_pin(dut->top_shadow_in, top_shadow_in, DATA_WIDTH);
        
        tick(dut->clk);

        vector<uint32_t> bottom_shadow_out = get_pin(dut->bottom_shadow_out, 1, DATA_WIDTH);
        EXPECT_EQ(rand_int, bottom_shadow_out[0]);

        top_shadow_in[0] = rand_int + 1;
        set_pin(dut->top_shadow_in, top_shadow_in, DATA_WIDTH);

        tick(dut->clk);

        bottom_shadow_out = get_pin(dut->bottom_shadow_out, 1, DATA_WIDTH);
        EXPECT_EQ(rand_int + 1, bottom_shadow_out[0]);

        tick(dut->clk);
    }
}
