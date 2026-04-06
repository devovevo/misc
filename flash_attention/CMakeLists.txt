cmake_minimum_required(VERSION 3.18)
project(FlashAttention LANGUAGES CXX CUDA)

add_executable(flash_attention_cuda flash_attention.cu)
add_executable(flash_attention_cpp flash_attention.cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# 1. Apply your custom DEBUG_MODE macro to both targets in Debug mode
target_compile_definitions(
    flash_attention_cpp
    PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE>
)

target_compile_definitions(
    flash_attention_cuda
    PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE>
)

target_compile_options(
    flash_attention_cpp
    PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer>
)

target_link_options(
    flash_attention_cpp
    PRIVATE
    $<$<CONFIG:DEBUG>:-fsanitize=address>
)

# 2. Explicitly set the debug postfix for both EXECUTABLES
set_target_properties(
    flash_attention_cpp flash_attention_cuda
    PROPERTIES 
    DEBUG_POSTFIX "_d"
)
