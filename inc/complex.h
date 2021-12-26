#include <math.h>
typedef struct
{
    double re;
    double im;
} complex_t;

static inline void init_complex_num(complex_t * const obj, 
                                    double re, double im)
{
    obj->re = re;
    obj->im = im;
}

static inline complex_t complex_mul(complex_t const * const a,
                                    complex_t const * const b)
{
    complex_t result = 
    {
        .re = a->re*b->re - a->im*b->im,
        .im = a->im*b->re + b->im*a->re,
    };

    return result;
}

static inline complex_t complex_add(complex_t const * const a,
                                    complex_t const * const b)
{
    complex_t result = 
    {
        .re = a->re + b->re,
        .im = a->im + b->im,
    };

    return result;
}

static inline complex_t complex_sub(complex_t const * const a,
                                    complex_t const * const b)
{
    complex_t result = 
    {
        .re = a->re - b->re,
        .im = a->im - b->im,
    };

    return result;
}
static inline void init_complex_num_expo(complex_t * const obj,
                                     double mag, double phase)
{
    obj->re = mag * cos(phase);
    obj->im = mag * sin(phase);
}

static inline double complex_abs(complex_t const * const obj)
{
    double result = obj->re * obj->re + obj->im * obj->im;

    return sqrt(result);
}
