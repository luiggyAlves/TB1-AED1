#ifndef EXPERIMENTOS_H
#define EXPERIMENTOS_H

#include <stdint.h>

#include "medicao.h"

/* Tamanhos de entrada exigidos na análise de desempenho. */
#define QTD_TAMANHOS 3
extern const int TAMANHOS_DESEMPENHO[QTD_TAMANHOS];

/* Posições (1..n) das consultas obrigatórias. */
int posicao_mediana(int n);      /* k = N/2 (arredondado para cima)  */
int posicao_percentil90(int n);  /* k = ⌈0,9·N⌉                     */

/* Seção 2: as três consultas obrigatórias pelas três abordagens. */
void executar_consultas(const Medicao *dados, int n, int m);

/* Seção 3: tabela comparativa (comparações e tempo) para N = 1.000,
   5.000 e 10.000 (os N primeiros registros de `dados`). */
void analisar_desempenho(const Medicao *dados, int n, int m);

/* Seção 4: melhor e pior caso de cada abordagem em cenários controlados. */
void analisar_casos(int n, uint64_t semente);

/* Seção 5: demonstrações das questões de acompanhamento prático. */
void questao1_rastreio_contagem(void);
void questao2_empates(void);
void questao3_escolha_do_pivo(const Medicao *dados, int n, uint64_t semente);
void questao5_reutilizacao(const Medicao *dados, int n);

#endif
