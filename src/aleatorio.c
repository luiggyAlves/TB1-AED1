#include "aleatorio.h"

#include <math.h>

void rng_semear(Rng *rng, uint64_t semente)
{
    rng->estado = semente;
}

uint64_t rng_proximo(Rng *rng)
{
    uint64_t z = (rng->estado += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

double rng_uniforme(Rng *rng)
{
    /* 53 bits aleatórios -> double em [0, 1) */
    return (double)(rng_proximo(rng) >> 11) * (1.0 / 9007199254740992.0);
}

size_t rng_intervalo(Rng *rng, size_t n)
{
    return (size_t)(rng_proximo(rng) % (uint64_t)n);
}

double rng_normal(Rng *rng)
{
    /* Transformação de Box-Muller */
    const double pi = 3.14159265358979323846;
    double u1 = 1.0 - rng_uniforme(rng);  /* (0, 1], evita log(0) */
    double u2 = rng_uniforme(rng);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * pi * u2);
}
