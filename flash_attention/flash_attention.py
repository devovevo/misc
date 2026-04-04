import torch
import torch.nn.functional as F

# 1. Setup dimensions (Matching your kernel's test case)
batch_size = 1
num_heads = 1
seq_len = 128
head_dim = 16

# 2. Initialize Matrices: Entries = Row Number
# We use .float() because your kernel likely uses float32
def row_init(rows, cols):
    return torch.arange(rows).view(-1, 1).repeat(1, cols).float() / seq_len

# Create Q, K, V (adding batch and head dims for PyTorch)
q = row_init(seq_len, head_dim).reshape(batch_size, num_heads, seq_len, head_dim)
k = row_init(seq_len, head_dim).reshape(batch_size, num_heads, seq_len, head_dim)
v = row_init(seq_len, head_dim).reshape(batch_size, num_heads, seq_len, head_dim)

# 3. Compute Reference Output
# is_causal=False matches your current "no mask" setup
# scale=1.0 if you aren't doing the 1/sqrt(dk) scaling yet
output_ref = F.scaled_dot_product_attention(q, k, v, is_causal=True)

print("PyTorch Reference Output (First few rows):")
print(output_ref[0, 0, ::, ::])
