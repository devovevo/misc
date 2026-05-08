# OOO CPU

I want to start experimenting with creating an Out of Order (OOO) CPU,
and seeing what's involved in the process. Here is my scratchpad.
I want to try and base this around RISCV32[I]

# L1 Cache

Based on some conversations with Gemini, I should make the SRAM width
of my L1 cache be the same as my cache line. I never truly considered
that this is why the cache line is the fundamental unit (I should have)
but now I see. Anyways, this makes sense. By setting up some
parameterizable Verilog for my mem controller I can switch out for banked,
BRAM, whatever. Would want the read latency, address width, and data width
all parameterizable

# Fetch

I'd probably want to make this parameterizable with the SRAM read latency,
address width, and data width, but also the output width, so how many
instructions to fetch at any given time and output. Implementation wise,
since L1 cache size may be bigger than what we want to fetch, we can even
fetch into a wide buffer and then mux out what we want. We could put an 
optional fetch buffer here, but I think it's best to leave that complexity
internal to the module

# Decode

We'd want our decode stage just as wide as our fetch. Seems like if we want
to truly get our maximum possible throughput we'd decode all the instructions
from fetch and get the party started. Most things would be direct mappings to
micro-ops, but we might also need to worry about mem ops (adding to get addr
then store/load). For this, maybe we'd need an extra cycle? It shouldn't really
matter.


