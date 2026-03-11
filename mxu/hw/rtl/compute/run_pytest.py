import sys
import os
import pytest
from bazel_tools.tools.python.runfiles import runfiles

if __name__ == "__main__":
    r = runfiles.Create()
    
    # Bzlmod adds a tilde (~) to external repository names. 
    # We check the standard Bzlmod name, the strict Bzlmod name, and legacy WORKSPACE.
    possible_paths = [
        "verilator~/bin/verilator",            # Standard Bzlmod
        "verilator~/verilator",                # Standard Bzlmod (Alternate)
        "verilator/bin/verilator",             # Legacy WORKSPACE
        "_main~ext~verilator/bin/verilator",   # Strict Bzlmod Ext
    ]
    
    hermetic_verilator = None
    for path in possible_paths:
        resolved = r.Rlocation(path)
        if resolved and os.path.exists(resolved):
            hermetic_verilator = resolved
            break

    if hermetic_verilator:
        v_bin_dir = os.path.dirname(hermetic_verilator)
        v_root = os.path.dirname(v_bin_dir)
        
        # Inject the hermetic toolchain into the PATH
        os.environ["PATH"] = v_bin_dir + os.pathsep + os.environ.get("PATH", "")
        os.environ["VERILATOR_ROOT"] = v_root
        
        print(f"\n--- BAZEL MAGIC: Found hermetic Verilator at {v_bin_dir} ---\n")
    else:
        print("\n--- FATAL: Hermetic Verilator not found in runfiles! ---\n")

    # Launch Pytest
    sys.exit(pytest.main(sys.argv[1:]))
