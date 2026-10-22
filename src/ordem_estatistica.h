#ifndef ORDEM_ESTATISTICA_H
#define ORDEM_ESTATISTICA_H

#include <stdbool.h>
#include <stdio.h>

#include "aleatorio.h"
#include "medicao.h"

/*
 * Ordem estatística: encontrar o valor que ocupa a posição `ordem`
 * (1 <= ordem <= tam) quando o vetor está em ordem crescente, ou seja,
 * o `ordem`-ésimo MENOR elemento. O k-ésimo MAIOR é o (tam - k + 1)-ésimo
 * menor.
 *
 * As funções seguem a estrutura do código-base da disciplina
 * (ordem__ultimaversao.c): bolha / oe_ordenacao, contagem / oe_contagem,
 * particionamento / oe_particionamento e o despachante ordem_estatistica,
 * que recebe a abordagem por ponteiro de função (t_funcao). As diferenças
 * em relação ao código-base são:
 *   - o vetor é de registros (Medicao) e a chave de comparação é escolhida
 *     pelo campo do Contexto (PM2.5, umidade ou temperatura);
 *   - toda comparação entre chaves é contada em ctx->comparacoes;
 *   - contagem e particionamento foram corrigidos para valores repetidos
 *     (ver comentários em ordem_estatistica.c e as Questões 2 e 3).
 *
 * Métrica de custo: UMA comparação = um confronto entre as chaves de dois
 * registros (função comparar(), que responde "menor", "igual" ou "maior").
 * Comparações entre índices/contadores não são comparações de dados.
 */

/* Escolha do pivô na Abordagem B. O trabalho exige o pivô central; as
   outras duas existem apenas para o experimento da Questão 3. */
typedef enum {
    PIVO_CENTRAL,
    PIVO_PRIMEIRO,
    PIVO_ALEATORIO
} EstrategiaPivo;

typedef struct {
    Campo              campo;        /* chave usada nas comparações       */
    unsigned long long comparacoes;  /* comparações de chaves realizadas  */
    EstrategiaPivo     pivo;         /* só para a Abordagem B             */
    Rng               *rng;          /* só para PIVO_ALEATORIO            */
    FILE              *rastreio;     /* se != NULL, a Abordagem C imprime
                                        seu passo a passo (Questão 1)     */
} Contexto;

/* Contexto zerado, pivô central e sem rastreio. */
void contexto_iniciar(Contexto *ctx, Campo campo);

/* Compara as chaves de a e b: <0, 0 ou >0. Conta uma comparação. */
int comparar(const Medicao *a, const Medicao *b, Contexto *ctx);

/* Assinatura comum às três abordagens (equivalente ao t_funcao do
   código-base, acrescida do contexto de medição). */
typedef bool (*t_funcao)(Medicao vetor[], int tam, int ordem,
                         Medicao *e_valor_para_ordem, Contexto *ctx);

/* Abordagem A: ordena todo o vetor com a bolha e acessa vetor[ordem-1]. */
void bolha(Medicao vetor[], int tam, Contexto *ctx);
bool oe_ordenacao(Medicao vetor[], int tam, int ordem,
                  Medicao *e_valor_para_ordem, Contexto *ctx);

/* Abordagem B: seleção por particionamento com vetor espelho e pivô
   central. Ao final, vetor[0..ordem-1] contém os `ordem` menores. */
bool oe_particionamento(Medicao vetor[], int tam, int ordem,
                        Medicao *e_valor_para_ordem, Contexto *ctx);

/* Abordagem C: contagem comparativa sequencial. Não altera o vetor. */
bool oe_contagem(Medicao vetor[], int tam, int ordem,
                 Medicao *e_valor_para_ordem, Contexto *ctx);

/* Despachante: executa a abordagem `chamadora`. */
bool ordem_estatistica(Medicao vetor[], int tam, int ordem,
                       Medicao *e_valor_para_ordem, t_funcao chamadora,
                       Contexto *ctx);

/*
 * "M dias mais secos" pela Abordagem C: uma única varredura que usa a
 * mesma contagem de oe_contagem para decidir, elemento a elemento, se ele
 * está entre as m primeiras posições. Copia esses registros para `saida`
 * (capacidade >= m) na ordem em que aparecem e retorna quantos copiou
 * (min(m, tam)).
 */
int listar_menores_contagem(Medicao vetor[], int tam, int m, Medicao saida[],
                            Contexto *ctx);

#endif
