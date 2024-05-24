#ifndef FFT_H
#define FFT_H

#include "rpi.h"

// int16 * int16 -> int32 multiply -- specific to arm_none_eabi
inline int32_t fft_fixed_mul(int16_t a, int16_t b) {
    int32_t result;
    asm volatile("SMULBB %0, %1, %2" : "=r"(result) : "r"(a), "r"(b));
    return result;
}

// converts uint32 centered at INT_MAX into int16 (Q.15) centered at 0
inline int16_t to_q15(uint32_t x) {
    // Step 1: Shift from [0, 2^18 - 1] to [-2^17, 2^17 - 1]
    // Subtracting 2^17 from the sample to center it around 0
    int32_t centered_value = x - (1 << 17);

    // Step 2: Now scale up to the 32-bit signed range
    // Multiplying by 2^14 to scale up to the [-2^31, 2^31 - 1] range
    // As we're in 32-bit signed integer, it will automatically be interpreted as [-1, 1) in Q.15 format
    int32_t scaled_value = centered_value << 14;

    // Step 3: Bitshift right to fit into 16 bits (Q.15 format)
    // We'll shift right by 16 bits to go from 32-bit to 16-bit
    int16_t q15_value = scaled_value >> 16;

    return q15_value;
}

// multiplies Q.15 * Q.15 into Q.15
inline int16_t fft_fixed_mul_q15(int16_t a, int16_t b) {
    int32_t c = fft_fixed_mul(a, b);
    // save the most significant bit that's lost (round up if set)
    int32_t round = (c >> 14) & 1;
    return (c >> 15) + round;
}

int32_t fft_fixed_cfft(int16_t *real, int16_t *imag, int16_t log2_len, unsigned inverse);
int32_t fft_fixed_rfft(int16_t *data, int32_t log2_len, unsigned inverse);

#endif // FFT_H