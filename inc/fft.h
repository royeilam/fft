#include <complex.h>

typedef float complex fft_output_t;
typedef float fft_input_t;

fft_output_t * fft(const fft_input_t d_input[], const size_t input_size, size_t * const output_size);
