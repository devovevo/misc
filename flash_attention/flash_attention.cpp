#include <limits>
#include <new>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <utility>
#include <cstring>
#include <stdlib.h>

const int block_size = 64;

template<typename T>
void load(const T* A, T* B, const int M, const int N, const int m, const int n, const int offset_i, const int offset_j) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            B[i * n + j] = A[(offset_i + i) * N + (offset_j + j)];
        }
    }
}

template<typename T>
void flash_attention(const T* Q, const T* K, const T* V, T* O, const int seq_len, const int head_dim) {
    T* q = (T*)std::aligned_alloc(std::hardware_constructive_interference_size, sizeof(T) * block_size * head_dim);
    T* k = (T*)std::aligned_alloc(std::hardware_constructive_interference_size, sizeof(T) * block_size * head_dim);
    T* v = (T*)std::aligned_alloc(std::hardware_constructive_interference_size, sizeof(T) * block_size * head_dim);
    T* s = (T*)std::aligned_alloc(std::hardware_constructive_interference_size, sizeof(T) * block_size * block_size);
    T* o = (T*)std::aligned_alloc(std::hardware_constructive_interference_size, sizeof(T) * block_size * head_dim);

    T* m = (T*)std::aligned_alloc(std::hardware_constructive_interference_size, sizeof(T) * block_size);
    T* l = (T*)std::aligned_alloc(std::hardware_constructive_interference_size, sizeof(T) * block_size);

    // Iterate over block rows
    for (int block_i = 0; block_i < seq_len; block_i += block_size) {
        load(Q, q, seq_len, head_dim, block_size, head_dim, block_i, 0);

        // Initialize our output, m, and l matrices
        std::fill(o, o + block_size * head_dim, 0);
        std::fill(m, m + block_size, -std::numeric_limits<T>::infinity());
        std::fill(l, l + block_size, 0);

        // Within block row iterate over seq length of V
        for (int block_j = 0; block_j < seq_len; block_j += block_size) {
            if (block_j > block_i)
                break;

            load(K, k, seq_len, head_dim, block_size, head_dim, block_j, 0);
            load(V, v, seq_len, head_dim, block_size, head_dim, block_j, 0);

            if (block_i == block_j) {
                // Now we need to compute block Q * block K
                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        if (j > i)
                            break;

                        s[i * block_size + j] = 0;

                        for (int d = 0; d < head_dim; d++) {
                            s[i * block_size + j] += q[i * head_dim + d] * k[j * head_dim + d];
                        }

                        s[i * block_size + j] /= std::sqrt(head_dim);
                    }
                }
            } else {
                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        s[i * block_size + j] = 0;

                        for (int d = 0; d < head_dim; d++) {
                            s[i * block_size + j] += q[i * head_dim + d] * k[j * head_dim + d];
                        }

                        s[i * block_size + j] /= std::sqrt(head_dim);
                    }
                }
            }

            // Update our current output matrix with the new stuff (we'll add the results after this)
            for (int i = 0; i < block_size; i++) {
                const int block_col_limit = (block_i == block_j) ? i + 1 : block_size;

                // Calculate block max so we can compute exps only once
                T m_block = -std::numeric_limits<T>::infinity();
                for (int j = 0; j < block_col_limit; j++) {
                    m_block = std::max(m_block, s[i * block_size + j]);
                }

                T m_old = m[i];
                T m_new = std::max(m_old, m_block);

                T alpha = std::exp(m_old - m_new);

                // Update the existing o results with the new scale factor
                for (int d = 0; d < head_dim; d++) {
                    o[i * head_dim + d] *= alpha;
                }

                // l temp within this row
                T l_temp = 0;
                for (int j = 0; j < block_col_limit; j++) {
                    T p = std::exp(s[i * block_size + j] - m_new);
                    l_temp += p;

                    for (int d = 0; d < head_dim; d++) {
                        o[i * head_dim + d] += p * v[j * head_dim + d];
                    }
                }
                        
                l[i] = (alpha * l[i]) + l_temp;
                m[i] = m_new;
            }
        }

        // Copy block O to O
        for (int i = 0; i < block_size; i++) {
            for (int d = 0; d < head_dim; d++) {
                O[(block_i + i) * head_dim + d] = o[i * head_dim + d] / l[i];
            }
        }
    }

    free(q);
    free(k);
    free(v);
    free(s);
    free(o);

    free(m);
    free(l);
}

int main() {
    const int seq_len = 128;
    const int head_dim = 16;

    float* Q = (float*)aligned_alloc(std::hardware_constructive_interference_size, sizeof(float) * seq_len * head_dim);
    float* K = (float*)aligned_alloc(std::hardware_constructive_interference_size, sizeof(float) * seq_len * head_dim);
    float* V = (float*)aligned_alloc(std::hardware_constructive_interference_size, sizeof(float) * seq_len * head_dim);
    float* O = (float*)aligned_alloc(std::hardware_constructive_interference_size, sizeof(float) * seq_len * head_dim);

    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < head_dim; d++) {
            Q[i * head_dim + d] = (float)i / seq_len;
            K[i * head_dim + d] = (float)i / seq_len;
            V[i * head_dim + d] = (float)i / seq_len;
        }
    }

    flash_attention(Q, K, V, O, seq_len, head_dim);

    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < head_dim; d++) {
            printf("%f ", O[i * head_dim + d]);
        }

        printf("\n");
    }

    free(Q);
    free(K);
    free(V);
    free(O);
}
