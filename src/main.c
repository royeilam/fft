#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "fft.h"


double * create_sine(size_t length, double freq, double fs)
{
    double * result = calloc(length, sizeof(double));
    double w = 0.0;
    size_t i = 0;

    if (result == NULL)
    {
        return NULL;
    }

    for (i = 0; i < length; i++)
    {
        result[i] = sin(w);
        w = fmod(w + 2*M_PI*freq/fs, 2*M_PI);
    }

    return result;
}

double get_max_freq(const double complex fft_data[], const size_t fft_len, const double fs)
{
    double max_abs = 0.0;
    size_t max_idx = 0;

    for (size_t i = 0; i < fft_len/2; i++)
    {
        double abs_val = cabs(fft_data[i]);
        if (abs_val > max_abs)
        {
            max_abs = abs_val;
            max_idx = i;
        }
    }

    return (double)max_idx/(fft_len/2.0) * fs/2;
}

int main(int argc, char * argv[])
{
    (void)argc;
    (void)argv;
    size_t result_size = 0;
    size_t my_sine_size = 2048;
    double freq = 100;
    double fs = 1E3;

    double * my_sine = NULL;
    double complex * fft_result = NULL;

    if (argc > 1)
    {
        freq = atof(argv[1]);
    }

    my_sine = create_sine(my_sine_size, freq, fs);
    fft_result = fft(my_sine, my_sine_size, &result_size);

    printf("Max freq = %lf\n", get_max_freq(fft_result, result_size, fs));

    free(my_sine);
    free(fft_result);
    return EXIT_SUCCESS;
}
