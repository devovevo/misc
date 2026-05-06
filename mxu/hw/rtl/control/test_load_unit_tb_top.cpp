#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "hw/shared/verilator_pins.h"
#include "hw/shared/verilator_test_fixture.h"
#include "Vload_unit_tb_top.h"

typedef Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXiRM;

using LoadUnitTest = VerilatorTestFixture<Vload_unit_tb_top>;

TEST_F(LoadUnitTest, RandomMatrixLoadLifecycle) {
    start_tracing("load_unit_rand_matrix_load_test.fst");

    const int READ_LATENCY = 3;
    const int DATA_WIDTH = 8;
    const int ADDR_WIDTH = 16;

    int max_val = (1 << DATA_WIDTH) - 1;

    MatrixXiRM W = MatrixXiRM::Random(SIZE, SIZE).cwiseAbs().unaryExpr(
            [max_val](int x) { return x % max_val; }
    );

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            cout << W(i, j) << " ";
        }

        cout << endl;
    }

    dut->rst_n = 0;
    dut->fifo_empty_in = 1;
    tick(dut->clk);
    dut->rst_n = 1;
    tick(dut->clk);

    dut->dma_we_in = 1;
    dut->dma_cs_in = 1;

    std::vector<uint32_t> dma_addr = {0};
    std::vector<uint32_t> dma_wdata(SIZE, 0);
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            dma_wdata[j] = W(i, j);
        }

        set_pin(dut->dma_addr_in, dma_addr, ADDR_WIDTH);
        set_pin(dut->dma_wdata_in, dma_wdata, DATA_WIDTH);
        tick(dut->clk);

        dma_addr[0] += 1;
    }

    dut->dma_we_in = 0;
    dut->dma_cs_in = 0;
    dut->dma_done_pulse_in = 1;
    tick(dut->clk);
    dut->dma_done_pulse_in = 0;

    int wait_for_dma = 0;
    uint32_t load_cmd = (wait_for_dma << 24) | (SIZE << 16) | 0; // base_addr is 0
    std::vector<uint32_t> fifo_data = {load_cmd};
    dut->fifo_empty_in = 0;
    set_pin(dut->fifo_data_in, fifo_data, 32);

    // Evaluate purely combinational logic (IDLE state responds to fifo_empty_in going low)
    dut->eval(); 

    EXPECT_EQ(dut->fifo_pop_out, 1)
        << "Expected fifo pop out to be true, but was not";
        
    tick(dut->clk); // Clock it, FSM should transition to POPPING
                    
    EXPECT_EQ(dut->fifo_pop_out, 0)
        << "Expected fifo pop out to be false, but was true";

    tick(dut->clk); // Clock again, FSM should transition to LOADING

    // We now need to wait for read latency cycles to saturate the pipeline and start reading
    for (int i = 0; i < READ_LATENCY; i++) {
        tick(dut->clk);
    }

    std::vector<uint32_t> top_shadow(SIZE, 0);
    for (int i = 0; i < SIZE; i++) {
        get_pin(dut->top_shadow_out, top_shadow, SIZE, DATA_WIDTH);
        for (int j = 0; j < SIZE; j++) {
            EXPECT_EQ(top_shadow[j], W(SIZE - i - 1, j)) << "Expected top shadow and W(SIZE - i - 1, j) to be equal with "
                << " i: " << i << " j: " << j << " whereas we actually"
                << " got top_shadow: " << top_shadow[j] << " and W(SIZE - i - 1, j): "
                << W(SIZE - i - 1, j);
        }

        tick(dut->clk);
    }
}

