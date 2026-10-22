#ifndef ALEATORIO_H
#define ALEATORIO_H

#include <stddef.h>
#include <stdint.h>

/*
 * Gerador pseudoaleatório próprio (SplitMix64). Usado em vez de rand() para
 * que a simulação e o pivô aleatório produzam exatamente os mesmos números
 * em qualquer compilador/sistema operacional (resultados reprodutíveis).
 */
typedef struct {
    uint64_t estado;
} Rng;

void     rng_semear(Rng *rng, uint64_t semente);
uint64_t rng_proximo(Rng *rng);
double   rng_uniforme(Rng *rng);              /* valor em [0, 1)        */
size_t   rng_intervalo(Rng *rng, size_t n);   /* inteiro em [0, n)      */
double   rng_normal(Rng *rng);                /* normal padrão N(0, 1)  */

#endif
