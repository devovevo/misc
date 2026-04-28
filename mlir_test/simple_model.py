import torch
# New binding locations for 2026 nightlies
from torch_mlir._mlir_libs import _mlir
from _mlir import ir, passmanager

from torch_mlir.extras.fx_importer import FxImporter
from torch_mlir.dialects import torch as torch_dialect
from torch_mlir.dialects import func, linalg, arith

# 1. Simple model for tracing
class MyModel(torch.nn.Module):
    def forward(self, x):
        return torch.relu(x + x)

model = MyModel()
example_arg = torch.randn(2, 2)

# 2. Context setup
context = ir.Context()
# Register the dialects so the IR understands 'torch', 'func', etc.
torch_dialect.register_dialect(context)
func.register_dialect(context)
linalg.register_dialect(context)
arith.register_dialect(context)

# 3. Import the FX graph
importer = FxImporter(context=context)
importer.import_stateless_graph(model, (example_arg,))
module = importer.module_op

# 4. Configure the Pass Manager for a full trace
# 'torch-backend-pipeline' is the standard "lowering" route
pm = passmanager.PassManager.parse(
    "builtin.module(torch-backend-pipeline)", 
    context=context
)

# Enable IR Printing: This is the magic TPU-style dump
context.enable_multithreading(False)
pm.enable_ir_printing() 

print("--- EXECUTING PASS-BY-PASS LOWERING ---")
pm.run(module)

# 5. Save final result to file
with open("pass_trace.mlir", "w") as f:
    f.write(module.get_asm())

print("\nSuccess! Trace printed above. Final IR saved to 'pass_trace.mlir'.")
