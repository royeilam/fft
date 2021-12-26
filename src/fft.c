#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "fft.h"

static inline size_t bit_reversal(size_t in, unsigned int max_bit)
{
    size_t out = 0;

    for (unsigned int i = 0; i < max_bit; i++)
    {
        if (in & (1U << i))
        {
            out |= 1U << (max_bit - i - 1);
        }
    }

    return out;
}

static inline unsigned int get_max_bit(size_t in)
{
    unsigned int i = 0;
    
    if (in == 0)
    {
        return 0;
    }

    for (i = 0; in; in>>=1U, i++);

    return i - 1;
}

static double complex * create_bit_reversal(const double d_input[], const size_t input_size, size_t * const output_size)
{
    double complex * result = NULL;

    *output_size = input_size;
    if (input_size & (input_size - 1))
    {
        unsigned int i;

        i = get_max_bit(input_size);
        *output_size = (1U << (i + 1));
    }

    result = calloc(*output_size, sizeof(double complex));
    if (result == NULL)
    {
        *output_size = 0;
        return NULL;
    }

    memset(result, 0, *output_size * sizeof(double complex));

    unsigned int max_bit = get_max_bit(*output_size);
    for (size_t i = 0; i < input_size; i++)
    {
        size_t idx = bit_reversal(i, max_bit);
        result[idx] = d_input[i];
    }

    
    return result;
}

double complex * fft(const double d_input[], const size_t input_size, size_t * const output_size)
{
    double complex * result;
    size_t s = 0;
    result = create_bit_reversal(d_input, input_size, output_size);

    for (s = 1; s <= get_max_bit(*output_size); s++)
    {
        size_t m = (1U << s);
        double complex w = 1;
        double complex w_m = cexp(I*2*M_PI/m);

        for (size_t j = 0; j < m/2; j++)
        {
            for (size_t k = j; k < *output_size; k+=m)
            {
                double complex t = w * result[k + m/2];
                double complex u = result[k];

                result[k] = u + t;
                result[k + m/2] = u - t;
            }

            w *= w_m;
        }
    }

    return result;
}
