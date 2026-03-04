mxu-accelerator/
├── Makefile                 # Top-level build commands (make sim, make lint, make clean)
├── README.md                # Architecture overview and ISA documentation
│
├── hw/                      # ALL HARDWARE (Silicon)
│   ├── interfaces/          # SystemVerilog Interfaces (The wire bundles)
│   │   ├── mxu_sram_if.sv   # SRAM read/write/addr interface
│   │   └── mxu_isa_if.sv    # The Command/Instruction bus interface
│   │
│   ├── rtl/                 # SYNTHESIZABLE SYSTEMVERILOG
│   │   ├── compute/         # The Math Datapath
│   │   │   ├── pe.sv        # Processing Element (1-cycle bubble MAC)
│   │   │   └── array.sv     # 2D generate loop of PEs
│   │   │
│   │   ├── memory/          # Storage and Wrappers
│   │   │   ├── sram_wrapper.sv # The `ifdef synthesis wrapper
│   │   │   └── sram_sim.sv  # The Verilator flip-flop memory
│   │   │
│   │   ├── dataflow/        # Moving and Shaping Data
│   │   │   ├── skew.sv      # Triangular input shift registers
│   │   │   └── deskew.sv    # Triangular output shift registers
│   │   │
│   │   ├── control/         # FSMs and Coordination
│   │   │   ├── load_fsm.sv  # FSM 1: SRAM -> Shadow Registers
│   │   │   ├── exec_fsm.sv  # FSM 2: SRAM -> Skew Buffer (Generates Valid Bits)
│   │   │   ├── drain_fsm.sv # FSM 3: Deskew -> Accumulator
│   │   │   └── delay_line.sv# The parallel Valid-Bit shift register
│   │   │
│   │   └── top/             # Integration
│   │       ├── mxu_core.sv  # Stitches array, skew, deskew, and control
│   │       └── mxu_top.sv   # The highest level: Core + SRAM blocks + External Pins
│   │
│   └── vendor/              # Third-party IP (Empty for now, but where TSMC macros go)
│
├── verif/                   # DESIGN VERIFICATION (Simulation)
│   ├── cpp/                 # Verilator C++ Testbenches
│   │   ├── sim_main.cpp     # The main Verilator loop and waveform dumper
│   │   └── mxu_driver.cpp   # A C++ class that formats tensors into ISA instructions
│   │
│   └── waves/               # Auto-generated .vcd or .fst waveform files (gitignored)
│
├── sw/                      # SOFTWARE STACK (The Compiler)
│   ├── compiler/
│   │   └── mxu_compiler.py  # Python script: takes PyTorch tensors, chunks them, outputs ISA binaries
│   └── runtime/             # C headers defining the memory map for the host CPU
│
└── scripts/                 # TOOLING
    ├── lint.sh              # Verilator strict linting script
    └── build_verilator.sh   # Compiles the C++ and SV into the executable simulator