import os
import random
from collections import deque
import pytest
import cocotb
from cocotb.clock import Clock
from cocotb_test.simulator import run
from cocotb.triggers import RisingEdge

# Default parameters (These will be overridden by the Pytest runner)
DWIDTH_DEFAULT = 8
AWIDTH_DEFAULT = 32

async def reset_dut(dut):
    """Helper to cleanly reset the PE."""
    dut.rst.value = 1
    dut.left_in.value = 0
    dut.top_in.value = 0
    dut.top_shadow_in.value = 0
    dut.load_enable_in.value = 0
    dut.left_weight_switch_in.value = 0
    
    await RisingEdge(dut.clock)
    await RisingEdge(dut.clock)
    
    dut.rst.value = 0
    await RisingEdge(dut.clock)

# 1. NO "test_" PREFIX HERE! Pytest will ignore this, leaving it for Cocotb.
@cocotb.test()
async def pe_continuous_streaming_test(dut):
    """Test the PE pipeline with continuous randomized data and weight swaps."""
    
    # Start a 10ns clock
    cocotb.start_soon(Clock(dut.clock, 10, units="ns").start())

    # Dynamically read the parameters from the compiled Verilog
    # (This ensures our Python math matches the hardware bit-widths)
    dwidth = int(dut.DWIDTH.value)
    awidth = int(dut.AWIDTH.value)
    
    max_d_val = (1 << dwidth) - 1
    acc_mask = (1 << awidth) - 1  # Used to simulate hardware overflow/truncation

    await reset_dut(dut)
    dut._log.info(f"=== STARTING PE TEST (DWIDTH={dwidth}, AWIDTH={awidth}) ===")

    # ----------------------------------------------------------------
    # Phase 1: Initial Weight Load
    # ----------------------------------------------------------------
    initial_weight = 5
    dut.load_enable_in.value = 1
    dut.top_shadow_in.value = initial_weight
    await RisingEdge(dut.clock)
    
    dut.load_enable_in.value = 0
    dut.left_weight_switch_in.value = 1
    await RisingEdge(dut.clock)
    
    dut.left_weight_switch_in.value = 0
    active_weight = initial_weight
    
    dut._log.info(f"Loaded initial weight: {active_weight}")

    # ----------------------------------------------------------------
    # Phase 2: Continuous Streaming & Concurrent Operations
    # ----------------------------------------------------------------
    # We use a deque (queue) to model the 1-cycle pipeline delay of the PE
    expected_rights = deque([0])  
    expected_bottoms = deque([0]) 

    for cycle in range(1, 21):
        # 1. Generate random inputs
        left_val = random.randint(0, max_d_val)
        top_val = random.randint(0, max_d_val * 2) # Keep it small to avoid massive prints, but it can be up to acc_mask

        # 2. Python Golden Model (Calculate what Verilog *should* do)
        expected_rights.append(left_val)
        
        # Calculate MAC and mask it to AWIDTH to simulate hardware wrapping
        mac_result = (active_weight * left_val) + top_val
        expected_bottoms.append(mac_result & acc_mask)

        # 3. Drive the Verilog inputs
        dut.left_in.value = left_val
        dut.top_in.value = top_val

        # --- CONCURRENCY INJECTION ---
        # On cycle 5, load a new shadow weight IN THE BACKGROUND
        if cycle == 5:
            dut._log.info("--> Loading new shadow weight (12) in the background...")
            dut.load_enable_in.value = 1
            dut.top_shadow_in.value = 12
        else:
            dut.load_enable_in.value = 0
            
        # On cycle 10, instantly swap to that new weight
        if cycle == 10:
            dut._log.info("--> SWAPPING active weight to 12!")
            dut.left_weight_switch_in.value = 1
            active_weight = 12 
        else:
            dut.left_weight_switch_in.value = 0

        # 4. Tick the clock!
        await RisingEdge(dut.clock)

        # 5. Catch outputs and assert against the Golden Model
        # Because of the pipeline, we are asserting the results of the PREVIOUS cycle
        actual_right = int(dut.right_out.value)
        actual_bottom = int(dut.bottom_out.value)
        
        exp_right = expected_rights.popleft()
        exp_bottom = expected_bottoms.popleft()

        dut._log.info(f"Cycle {cycle:02d} | In: L={left_val:03d}, T={top_val:04d} | Out: R={actual_right:03d} (Exp {exp_right:03d}), B={actual_bottom:05d} (Exp {exp_bottom:05d})")

        assert actual_right == exp_right, f"Right out mismatch! Expected {exp_right}, got {actual_right}"
        assert actual_bottom == exp_bottom, f"Bottom out mismatch! Expected {exp_bottom}, got {actual_bottom}"

    dut._log.info("SUCCESS! PE computed perfectly across all cycles and concurrent swaps.")


# 2. THE PYTEST RUNNER (This is what Bazel/Pytest actually executes)
@pytest.mark.parametrize("dwidth", [DWIDTH_DEFAULT, 16]) # Test 8-bit and 16-bit data
@pytest.mark.parametrize("awidth", [AWIDTH_DEFAULT, 64]) # Test 32-bit and 64-bit accumulators
def test_pe_runner(dwidth, awidth):
    """Compiles the Verilog with specific parameters and hands off to Cocotb."""
    
    pe_file = os.path.join(os.path.dirname(__file__), "pe.sv")

    # BAZEL MAGIC: Export waveforms to the testlogs directory
    bazel_output_dir = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR", "sim_build")

    run(
        verilog_sources = [pe_file],
        toplevel = "pe",
        module = "test_pe", # MUST match this Python file's name!
        
        # Inject the Pytest parameters into the Verilog compilation
        parameters = {
            "DWIDTH": str(dwidth),
            "AWIDTH": str(awidth)
        },
        
        simulator = "verilator",
        sim_build = bazel_output_dir,
        waves = True, # Enable waveform dumping
        extra_args = ["--trace-fst", "--trace-structs"],
        compile_args = ["-Wno-WIDTH"]
    )
