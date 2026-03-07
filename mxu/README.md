|  HBM |
    |    - |    DMA Unit    |
| SRAM | - | Systolic Array |
    |
|  VPU |

We want to use either TileLink or AMBA AXI between SRAM and the compute units
More than likely need an arbiter between VPU and systolic array, always giving systolic array feed unit preference

Zoom in on SRAM, we'd have

| Bank0 | Bank1 | Bank2 | Bank3 |

We want to simulate DWIDTH * SIZE memory width. To do this,
we split it evenly among the banks then read the same address all at once and combine them
