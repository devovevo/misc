#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <random>
#include "hw/shared/verilator_pins.h"
#include "hw/shared/verilator_test_fixture.h"
#include "Vflip_flop_sram.h"

using FlipFlopSRAMTest = VerilatorTestFixture<Vflip_flop_sram>;

TEST_F(FlipFlopSRAMTest, RandomReadWriteLifeCycle) {
    start_tracing("flip_flop_sram_rand_write_read");

    const int READ_LATENCY = 3;
    const int DATA_WIDTH = 8;
    const int ADDR_WIDTH = 16;
    int max_value = (1 << DATA_WIDTH) - 1;
    int max_addr = (1 << ADDR_WIDTH) - 1;

    dut->rst_n = 0;
    tick(dut->clk);
    dut->rst_n = 1;
    tick(dut->clk);

    dut->cs_in = 1;
    dut->we_in = 1;

    vector<uint32_t> wdata = {0};
    vector<uint32_t> addr = {0};
    vector<uint32_t> rdata = {0};

    std::random_device rd;
    
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> data_distrib(0, max_value);
    std::uniform_int_distribution<int> addr_distrib(0, max_addr);

    const int NUM_TESTS = 100;
    for (int i = 0; i < NUM_TESTS; i++) {
        unsigned int rand_int = data_distrib(gen);
        wdata[0] = rand_int;

        unsigned int rand_addr = addr_distrib(gen);
        addr[0] = rand_addr;

        set_pin(dut->addr_in, addr, ADDR_WIDTH);
        set_pin(dut->wdata_in, wdata, DATA_WIDTH);

        tick(dut->clk);

        for (int i = 0; i < READ_LATENCY; i++) {
            tick(dut->clk);
        }

        get_pin(dut->rdata_out, rdata, 1, DATA_WIDTH);
        EXPECT_EQ(rand_int, rdata[0])
            << "Expected rand_int: " << rand_int << " and rdata: " << rdata[0]
            << " to be equal";

        tick(dut->clk);
    }
}
