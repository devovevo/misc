#include <cstdint>
#include <type_traits>
#include <vector>

using namespace std;

// --- THE MAGICAL C++17 PACKER (UB PATCHED) ---
template <typename T>
void set_pin(T& pin, const std::vector<uint32_t>& arr, int width) {
    if constexpr (std::is_integral_v<T>) {
        uint64_t packed = 0;
        for (size_t i = 0; i < arr.size(); i++) {
            // FIXED: Using 1ULL for 64-bit safe shifts
            uint64_t val = arr[i] & ((1ULL << width) - 1); 
            packed |= (val << (i * width));
        }
        pin = static_cast<T>(packed);
    } 
    else {
        for(size_t j = 0; j < sizeof(T)/sizeof(uint32_t); j++) pin[j] = 0;
        for (size_t i = 0; i < arr.size(); i++) {
            int bit_offset = i * width;
            int word_idx = bit_offset / 32;
            int bit_shift = bit_offset % 32;
            
            uint32_t val = arr[i] & ((1ULL << width) - 1);
            pin[word_idx] |= (val << bit_shift);
            if (bit_shift + width > 32) {
                pin[word_idx + 1] |= (val >> (32 - bit_shift));
            }
        }
    }
}

// --- THE MAGICAL C++17 UNPACKER (UB PATCHED) ---
template <typename T>
std::vector<uint32_t> get_pin(const T& pin, int num_elements, int width) {
    std::vector<uint32_t> res(num_elements, 0);
    if constexpr (std::is_integral_v<T>) {
        uint64_t packed = static_cast<uint64_t>(pin);
        for (int i = 0; i < num_elements; i++) {
            res[i] = (packed >> (i * width)) & ((1ULL << width) - 1);
        }
    } else {
        for (int i = 0; i < num_elements; i++) {
            int bit_offset = i * width;
            int word_idx = bit_offset / 32;
            int bit_shift = bit_offset % 32;
            
            uint32_t val = pin[word_idx] >> bit_shift;
            if (bit_shift + width > 32) {
                val |= (pin[word_idx + 1] << (32 - bit_shift));
            }
            res[i] = val & ((1ULL << width) - 1);
        }
    }
    return res;
}
