#include <__clang_cuda_builtin_vars.h>
#include <__clang_cuda_runtime_wrapper.h>
#include <limits>
#include <stdio.h>
#include <mma.h>
#include <cuda/pipeline>
#include <cooperative_groups.h>
#include <cooperative_groups/memcpy_async.h>

namespace cg = cooperative_groups;
using namespace nvcuda;

const int WMMA_M = 16;
const int WMMA_K = 16;
const int WMMA_N = 16;

const int WARP_SIZE = 32;

template<typename T, const int BLOCK_SIZE_M, const int BLOCK_SIZE_N, const int PIPELINE_LEN>
__global__ void flash_attention(const T* Q, const T* K, const T* V, T* O, int seq_len, int head_dim) {
    extern __shared__ T m[];

    T* q = m;
    T* k = q + BLOCK_SIZE_M * head_dim;
    T* v = k + BLOCK_SIZE_N * head_dim * PIPELINE_LEN;

    cg::thread_block block = cg::this_thread_block();

    __shared__ cuda::pipeline_shared_state<cuda::thread_scope_block, 1> shared_q_load_pipeline_state;
    auto q_pipe = cuda::make_pipeline(block, &shared_q_load_pipeline_state);

    __shared__ cuda::pipeline_shared_state<cuda::thread_scope_block, PIPELINE_LEN> shared_k_load_pipeline_state;
    auto k_pipe = cuda::make_pipeline(block, &shared_k_load_pipeline_state);

    __shared__ cuda::pipeline_shared_state<cuda::thread_scope_block, PIPELINE_LEN> shared_v_load_pipeline_state;
    auto v_pipe = cuda::make_pipeline(block, &shared_v_load_pipeline_state);

    size_t blockStride = gridDim.x * blockDim.x;
    for (int block_i = 0; block_i < seq_len; block_i += blockStride) {
        // Prepare the Q load - just one
        q_pipe.producer_acquire();

        const int global_row_start = blockIdx.x * blockDim.x + block_i;
        const T* global_block_q_ptr = &Q[global_row_start * head_dim];

        size_t q_tile_bytes = BLOCK_SIZE_M * head_dim * sizeof(T);
        cuda::memcpy_async(block, q, global_block_q_ptr, cuda::aligned_size_t<16>(q_tile_bytes), q_pipe);

        q_pipe.producer_commit();

        // Fill the K and V pipeline loads
        const int tile_elems = BLOCK_SIZE_N * head_dim;
        for (int p = 0; p < PIPELINE_LEN; p++) {
            k_pipe.producer_acquire();
            v_pipe.producer_acquire();

            const T* global_block_k_ptr = &K[p * BLOCK_SIZE_N * head_dim];
            const T* global_block_v_ptr = &V[p * BLOCK_SIZE_N * head_dim];

            cuda::memcpy_async(block, global_block_k_ptr, sizeof(T) * tile_elems);
            cuda::memcpy_async(block, global_block_v_ptr, sizeof(T) * tile_elems);

            k_pipe.producer_commit();
            v_pipe.producer_acquire();
        }

        wmma::fragment<wmma::matrix_a, WMMA_M, WMMA_N, WMMA_K, T, wmma::row_major> q_frag;
        wmma::fragment<wmma::matrix_b, WMMA_M, WMMA_N, WMMA_K, T, wmma::col_major> k_frag;
        wmma::fragment<wmma::matrix_b, WMMA_M, WMMA_N, WMMA_K, T, wmma::row_major> v_frag;
        wmma::fragment<wmma::accumulator, WMMA_M, WMMA_N, WMMA_K, T> acc_s;
        wmma::fragment<wmma::accumulator, WMMA_M, WMMA_N, WMMA_K, T> acc_o;

        wmma::fill_fragment(acc_o, 0);
        T m = -cuda::std::numeric_limits<T>::infinity();
        T l = 0;

        q_pipe.consumer_wait();

        int read_idx = 0;
        for (int block_j = 0; block_j < seq_len; block_j += BLOCK_SIZE_N) {
            k_pipe.consumer_wait();

            T* k_curr = k[tile_elems * (read_idx % PIPELINE_LEN)];
            k_pipe.consumer_wait();

            const int warp_idx = threadIdx.x / WARP_SIZE;
            const int warp_row_offset = warp_idx * WMMA_M;

            for (int warp_j = 0; warp_j < BLOCK_SIZE_N; warp_j += WMMA_N) {
                wmma::fill_fragment(acc_s, 0);

                for (int warp_k = 0; warp_k < head_dim; warp_k += WMMA_K) {
                    wmma::load_matrix_sync(q_frag, q + (warp_row_offset * head_dim) + warp_k, head_dim);
                    wmma::load_matrix_sync(k_frag, k + (warp_j * head_dim) + warp_k, head_dim);
                    wmma::mma_sync(acc_s, q_frag, k_frag, acc_s);
                }

                // We store the ouput in K since we no longer need it
                // Assumes head_dim > BLOCK_SIZE_N which is reasonable
                T* warp_write_ptr = k_curr + (warp_row_offset * head_dim) + warp_j;
                wmma::store_matrix_sync(warp_write_ptr, acc_s, BLOCK_SIZE_N, wmma::mem_row_major); 
            }

            const int rows_per_warp = WMMA_M;
            const int threads_per_row = WARP_SIZE / rows_per_warp;
            const int lane_id = threadIdx.x % WARP_SIZE;

            const int local_row = lane_id / threads_per_row;
            const int local_col_offset = lane_id % threads_per_row;

            T* row_ptr = k + warp_row_offset + local_row;

            T local_m = -cuda::std::numeric_limits<T>::infinity();
            for (int col = local_col_offset; col < BLOCK_SIZE_M; col += threads_per_row) {
                local_m = fmaxf(local_m, row_ptr[col]);
            }

            for (int offset = threads_per_row / 2; offset > 0; offset /= 2) {
                T neighbor_m = __shuffle_down_sync(0xFFFFFFFF, local_m, offset, threads_per_row);
                local_m = fmaxf(local_m, neighbor_m);
            }

            T row_m = __shfl_sync(0xFFFFFFFF, local_m, 0, threads_per_row);
        }
    }
}

int main() {
    printf("Starting kernel\n");

    cudaDeviceSynchronize();

    return 0;
}
