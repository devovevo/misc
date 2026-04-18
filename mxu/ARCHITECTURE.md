.
├── ARCHITECTURE.md
├── BUILD.bazel
├── hw
│   ├── interfaces
│   │   └── sram_if.sv
│   ├── packages
│   │   └── mxu_pkg.sv
│   ├── rtl
│   │   ├── compute
│   │   │   ├── BUILD.bazel
│   │   │   ├── pe.sv
│   │   │   ├── README.md
│   │   │   ├── systolic_array.sv
│   │   │   ├── test_pe.cpp
│   │   │   └── test_systolic_array.cpp
│   │   ├── control
│   │   │   ├── BUILD.bazel
│   │   │   ├── compute_fsm.sv
│   │   │   ├── load_fsm.sv
│   │   │   ├── load_unit_tb_top.sv
│   │   │   └── test_load_fsm.cpp
│   │   ├── dataflow
│   │   │   ├── deskew.sv
│   │   │   └── skew.sv
│   │   └── memory
│   │       └── flip_flop_sram.sv
│   └── shared
│       ├── BUILD.bazel
│       ├── verilator_pins.h
│       └── verilator_test_fixture.h
├── main.py
├── MODULE.bazel
├── MODULE.bazel.lock
├── pe_rand_load.fst
├── pyproject.toml
├── README.md
├── requirements_lock.txt
├── sim_main.cpp
├── systolic_array_rand_matmul_test.fst
├── uv.lock
└── waveform.fst

14 directories, 32 files

