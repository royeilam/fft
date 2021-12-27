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

static fft_output_t * create_bit_reversal(const fft_input_t d_input[], const size_t input_size, size_t * const output_size)
{
    fft_output_t * result = NULL;

    *output_size = input_size;
    if (input_size & (input_size - 1))
    {
        unsigned int i;

        i = get_max_bit(input_size);
        *output_size = (1U << (i + 1));
    }

    result = calloc(*output_size, sizeof(fft_output_t));
    if (result == NULL)
    {
        *output_size = 0;
        return NULL;
    }

    memset(result, 0, *output_size * sizeof(fft_output_t));

    unsigned int max_bit = get_max_bit(*output_size);
    for (size_t i = 0; i < input_size; i++)
    {
        size_t idx = bit_reversal(i, max_bit);
        result[idx] = d_input[i];
    }

    
    return result;
}

fft_output_t * fft(const fft_input_t d_input[], const size_t input_size, size_t * const output_size)
{
    fft_output_t * result;
    size_t s = 0;
    result = create_bit_reversal(d_input, input_size, output_size);

    for (s = 1; s <= get_max_bit(*output_size); s++)
    {
        size_t m = (1U << s);
        fft_output_t w = 1;
        fft_output_t w_m = cexpf(I*2*M_PI/m);

        for (size_t j = 0; j < m/2; j++)
        {
            for (size_t k = j; k < *output_size; k+=m)
            {
                fft_output_t t = w * result[k + m/2];
                fft_output_t u = result[k];

                result[k] = u + t;
                result[k + m/2] = u - t;
            }

            w *= w_m;
        }
    }

    return result;
}
