#include <stdio.h>

template<typename T>
__global__ void flash_attention(const T* Q, const T* K, const T* V, T* O, int seq_len, int d) {

}

int main() {
    printf("Starting kernel\n");


    cudaDeviceSynchronize();

    return 0;
}
