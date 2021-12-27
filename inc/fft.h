#include <complex.h>

typedef float complex fft_output_t;
typedef float fft_input_t;

fft_output_t * create_fft_data(size_t * const length);
void fft(const fft_input_t in_data[], const size_t in_len,
         fft_output_t out_data[], const size_t out_len);
