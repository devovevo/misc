#pragma once
#include <gtest/gtest.h>
#include <verilated.h>
#include <verilated_fst_c.h>
#include <memory>
#include <string>

using namespace std;

inline double sc_time_stamp() { return 0; }

template <typename TDUT>
class VerilatorTestFixture : public ::testing::Test {
protected:
    shared_ptr<VerilatedContext> contextp;
    shared_ptr<VerilatedFstC> tfp;
    shared_ptr<TDUT> dut;

    void SetUp() override {
        contextp = make_unique<VerilatedContext>();
        contextp->traceEverOn(true);
        dut = make_unique<TDUT>(contextp.get());
        Verilated::randReset(2);
    }

    void TearDown() override {
        dut->final();
        if (tfp) {
            tfp->close();
        }
    }

    void start_tracing(const string& vcd_filename) {
        string output_path = vcd_filename;
        if (const char* env_dir = getenv("TEST_UNDECLARED_OUTPUTS_DIR")) {
            output_path = string(env_dir) + "/" + vcd_filename;
        }

        tfp = make_unique<VerilatedFstC>();
        dut->trace(tfp.get(), 99);
        tfp->open(output_path.c_str());
    }

    void step() {
        dut->eval();
        
        if (tfp) {
            tfp->dump(contextp->time());
        }

        contextp->timeInc(1);
    }


    void tick(unsigned char& clk_pin, int cycles = 1) {
        for (int i = 0; i < cycles; i++) {
            clk_pin = 0;
            step();

            clk_pin = 1;
            step();
        }
    }
};
