#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "hw/shared/verilator_pins.h"
#include "hw/shared/verilator_test_fixture.h"
#include "Vload_fsm_tb_top.h"

using LoadUnitTest = VerilatorTestFixture<Vload_fsm_tb_top>;

TEST_F(LoadUnitTest, RandomMatrixLoadLifecycle) {
    start_tracing("load_unit_rand_matrix_load_test.fst");

    const int DATA_WIDTH = 8;
    int max_val = (1 << DATA_WIDTH) - 1;

    Eigen::MatrixXi W = Eigen::MatrixXi::RANDOM(SIZE, SIZE).cwiseAbs().unaryExpr(
            [max_val](int x) { return x % max_val; }
    );

    dut->rst_n = 0;
    tick(dut->clk);
    dut->rst_n = 1;
    tick(dut->clk);

    dut->dma_we_in = 1;

    for (int i = 0; i < SIZE * SIZE; i++) {
        std::vector<uint32_t> dma_addr = {i};
        std::vector<uint32_t> dma_wdata = W(i);
}

