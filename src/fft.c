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

fft_output_t * create_fft_data(size_t * const length)
{
    if (*length &(*length - 1))
    {
        unsigned int i;

        i = get_max_bit(*length);
        *length = (1U << (i + 1));
    }

    return calloc(*length, sizeof(fft_output_t));
}

static void create_bit_reversal(const fft_input_t in_data[], const size_t in_len,
                                fft_output_t out_data[], const size_t out_len)
{
    memset(out_data + in_len, 0, (out_len - in_len) * sizeof(fft_output_t));

    unsigned int max_bit = get_max_bit(out_len);
    for (size_t i = 0; i < in_len; i++)
    {
        size_t idx = bit_reversal(i, max_bit);
        out_data[idx] = in_data[i];
    }
}

void fft(const fft_input_t in_data[], const size_t in_len,
         fft_output_t out_data[], const size_t out_len)
{
    size_t s = 0;
    create_bit_reversal(in_data, in_len, out_data, out_len);

    for (s = 1; s <= get_max_bit(out_len); s++)
    {
        size_t m = (1U << s);
        fft_output_t w = 1;
        fft_output_t w_m = cexpf(I*2*M_PI/m);

        for (size_t j = 0; j < m/2; j++)
        {
            for (size_t k = j; k < out_len; k+=m)
            {
                fft_output_t t = w * out_data[k + m/2];
                fft_output_t u = out_data[k];

                out_data[k] = u + t;
                out_data[k + m/2] = u - t;
            }

            w *= w_m;
        }
    }
}
