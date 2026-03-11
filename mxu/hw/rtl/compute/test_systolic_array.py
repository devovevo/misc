import os
import pytest
import numpy as np
import cocotb
from cocotb.clock import Clock
from cocotb_test.simulator import run
from cocotb.triggers import RisingEdge

SIZE = 3
DATA_WIDTH = 8
ACC_WIDTH = 32

def pack_array_to_int(numpy_array, width_per_element):
    """Packs a NumPy array into a single integer for Verilog packed ports."""
    packed_val = 0
    for i, val in enumerate(numpy_array):
        # Mask the value to ensure it fits in the bit width, then shift it
        masked_val = int(val) & ((1 << width_per_element) - 1)
        packed_val |= (masked_val << (i * width_per_element))
    return packed_val

def unpack_int_to_array(packed_val, num_elements, width_per_element):
    """Unpacks a single Verilog integer back into a NumPy array."""
    unpacked = np.zeros(num_elements, dtype=np.int32)
    mask = (1 << width_per_element) - 1
    
    try:
        val = int(packed_val)
    except ValueError:
        val = 0 # If the simulator outputs 'x' or 'z' early on, treat as 0
        
    for i in range(num_elements):
        unpacked[i] = (val >> (i * width_per_element)) & mask
    return unpacked

# Notice: No "test_" prefix here so Pytest doesn't hijack it!
@cocotb.test()
async def systolic_array_lifecycle_test(dut):
    """Test loading array, streaming data, and deskewing outputs."""
    
    cocotb.start_soon(Clock(dut.clock, 10, units="ns").start())

    # Generate random matrices
    max_data = (1 << DATA_WIDTH) - 1
    A = np.random.randint(low=0, high=max_data, size=(SIZE, SIZE))
    B = np.random.randint(low=0, high=max_data, size=(SIZE, SIZE))
    
    # Calculate the Golden Model Result
    C_expected = np.dot(A, B)

    dut._log.info("\n=== STARTING SYSTOLIC ARRAY TEST ===")
    dut._log.info(f"Matrix A (Activations):\n{A}")
    dut._log.info(f"Matrix B (Weights):\n{B}")
    dut._log.info(f"Expected Matrix C:\n{C_expected}")

    # Reset (Wait 1 cycle)
    await RisingEdge(dut.clock)

    # ----------------------------------------------------------------
    # Step 1: Load Weights (Pumped from the top)
    # ----------------------------------------------------------------
    dut._log.info("\n--- Phase 1: Loading Weights ---")
    for i in range(SIZE):
        # Feed bottom rows of B first so they shift down to the bottom PEs
        row_to_load = B[SIZE - i - 1, :]
        dut.top_shadow_in.value = pack_array_to_int(row_to_load, DATA_WIDTH)
        dut.top_load_enable_in.value = (1 << SIZE) - 1  # All 1s
        
        dut._log.info(f"Tick: Pushing weight row {SIZE - i - 1} into top shadows...")
        await RisingEdge(dut.clock)

    # Turn off load enable
    dut.top_load_enable_in.value = 0
    await RisingEdge(dut.clock)

    # ----------------------------------------------------------------
    # Step 2 & 3: Stream Activations, Switch Weights, and Catch Outputs
    # ----------------------------------------------------------------
    dut._log.info("\n--- Phase 2: Streaming Activations and Catching Outputs ---")
    
    # We need 3 * SIZE cycles to push the wavefront entirely through the array
    TOTAL_CYCLES = 3 * SIZE 
    deskewed_C = np.zeros((SIZE, SIZE), dtype=np.int32)

    for t in range(TOTAL_CYCLES):
        left_in_array = np.zeros(SIZE, dtype=np.int32)
        switch_in_array = np.zeros(SIZE, dtype=int)

        # 2A. Prepare the skewed inputs for this cycle
        for r in range(SIZE):
            # The weight switch must trigger exactly when the first data arrives (t == r)
            if t == r:
                switch_in_array[r] = 1

            # Skew logic: Row r needs Col r of A. A[i, r] enters Row r at cycle t = i + r
            i = t - r
            if 0 <= i < SIZE:
                left_in_array[r] = A[i, r]

        # Drive the physical hardware
        dut.left_in.value = pack_array_to_int(left_in_array, DATA_WIDTH)
        dut.left_switch_in.value = pack_array_to_int(switch_in_array, 1)

        # TICK THE CLOCK!
        await RisingEdge(dut.clock)
        cycle_count = t + 1 # The cycle we just finished
        
        # 3A. Read and deskew the outputs
        out_array = unpack_int_to_array(dut.bottom_out.value, SIZE, ACC_WIDTH)
        
        for c in range(SIZE):
            # Deskew logic: C[i, c] emerges from column c at cycle: i + SIZE + c
            # Therefore, i = cycle - SIZE - c
            i = cycle_count - SIZE - c
            
            if 0 <= i < SIZE:
                deskewed_C[i, c] = out_array[c]
                dut._log.info(f" Cycle {cycle_count:02d} | Caught C[{i},{c}] = {out_array[c]}")
                
        # Optional: Print raw pipeline state if you want to watch the wavefront
        # dut._log.debug(f"Cycle {cycle_count}: In={left_in_array}, Out={out_array}")

    # ----------------------------------------------------------------
    # Step 4: Final Assertion
    # ----------------------------------------------------------------
    dut._log.info("\n--- Phase 3: Final Verification ---")
    dut._log.info(f"Hardware Computed Matrix C:\n{deskewed_C}")
    
    assert np.array_equal(C_expected, deskewed_C), "Hardware computation did not match Golden Model!"
    dut._log.info("SUCCESS! Systolic array computed the exact correct matrix.")

@pytest.mark.parametrize("size", [SIZE])
@pytest.mark.parametrize("data_width", [DATA_WIDTH])
@pytest.mark.parametrize("acc_width", [ACC_WIDTH])
def test_systolic_array_runner(size, data_width, acc_width):
    systolic_array_file = os.path.join(os.path.dirname(__file__), "systolic_array.sv")
    pe_file = os.path.join(os.path.dirname(__file__), "pe.sv")

    # BAZEL MAGIC: Ask Bazel where we are allowed to save permanent files.
    # If we aren't running inside Bazel, fallback to a local "sim_build" folder.
    bazel_output_dir = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR", "sim_build")

    run(
        verilog_sources = [systolic_array_file, pe_file],
        toplevel = "systolic_array",
        module = "test_systolic_array", 
        parameters = {
            "SIZE": str(size),
            "DATA_WIDTH": str(data_width),
            "ACC_WIDTH": str(acc_width)
        },
        simulator = "verilator",
        
        # 1. Tell cocotb-test to build the simulation in the Bazel outputs folder
        sim_build = bazel_output_dir,
        
        # 2. Tell Cocotb to dump the waves
        waves = True,
        
        # 3. Tell Verilator to instrument the C++ for Fast Signal Trace (FST) 
        # and include structs/multidimensional arrays
        extra_args = ["--trace-fst", "--trace-structs"],
        compile_args = ["-Wno-WIDTH"]
    )
